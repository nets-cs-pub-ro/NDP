//-
// Copyright (C) 2010, 2011 The Board of Trustees of The Leland Stanford
//                          Junior University
// Copyright (C) 2010, 2011 Muhammad Shahbaz
// Copyright (C) 2015 Gianni Antichi, Noa Zilberman
// Copyright (C) 2016 Gianni Antichi, Marcin Wójcik
// All rights reserved.
//
// This software was developed by
// Stanford University and the University of Cambridge Computer Laboratory
// under National Science Foundation under Grant No. CNS-0855268,
// the University of Cambridge Computer Laboratory under EPSRC INTERNET Project EP/H040536/1 and
// by the University of Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-11-C-0249 ("MRC2"),
// as part of the DARPA MRC research programme.
//
// @NETFPGA_LICENSE_HEADER_START@
//
// Licensed to NetFPGA C.I.C. (NetFPGA) under one or more contributor
// license agreements.  See the NOTICE file distributed with this work for
// additional information regarding copyright ownership.  NetFPGA licenses this
// file to you under the NetFPGA Hardware-Software License, Version 1.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at:
//
//   http://www.netfpga-cic.org
//
// Unless required by applicable law or agreed to in writing, Work distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations under the License.
//
// @NETFPGA_LICENSE_HEADER_END@
//
/*******************************************************************************
 *  File:
 *        nf_ndp.v
 *
 *  Library:
 *        hw/contrib/cores/nf_ndp
 *
 *  Module:
 *        nf_ndp
 *
 *  Author:
 *        Gianni Antichi, Muhammad Shahbaz
 *        Modified by Noa Zilberman, Marcin Wójcik
 *
 *  Description:
 *        NDP switch core logic
 *
 */


module nf_ndp #(
    /* Master AXI Stream Data Width */
    parameter C_M_AXIS_DATA_WIDTH              = 256,
    parameter C_S_AXIS_DATA_WIDTH              = 256,
    parameter C_M_AXIS_TUSER_WIDTH             = 128,
    parameter C_S_AXIS_TUSER_WIDTH             = 128,
    parameter NUM_OUTPUT_QUEUES                = 8,

    /* AXI Registers Data Width */
    parameter C_S_AXI_DATA_WIDTH               = 32,
    parameter C_S_AXI_ADDR_WIDTH               = 12,
    parameter C_BASEADDR                       = 32'h00000000,
    parameter C_HIGHADDR                       = 32'h0000FFFF
) (
    /* Global Ports */
    input                                      axis_aclk,
    input                                      axis_resetn,

    /* Master Stream Ports (interface to data path) */
    output [C_M_AXIS_DATA_WIDTH - 1:0]         m_axis_tdata,
    output [((C_M_AXIS_DATA_WIDTH / 8)) - 1:0] m_axis_tkeep,
    output [C_M_AXIS_TUSER_WIDTH-1:0]          m_axis_tuser,
    output                                     m_axis_tvalid,
    input                                      m_axis_tready,
    output                                     m_axis_tlast,

    /* Slave Stream Ports (interface to RX queues) */
    input [C_S_AXIS_DATA_WIDTH - 1:0]          s_axis_tdata,
    input [((C_S_AXIS_DATA_WIDTH / 8)) - 1:0]  s_axis_tkeep,
    input [C_S_AXIS_TUSER_WIDTH-1:0]           s_axis_tuser,
    input                                      s_axis_tvalid,
    output                                     s_axis_tready,
    input                                      s_axis_tlast,

    /* HQs Full */
    input H_0_full,
    input H_1_full,
    input H_2_full,
    input H_3_full,
    input H_4_full,

    /* PQs Full */
    input P_0_full,
    input P_1_full,
    input P_2_full,
    input P_3_full,
    input P_4_full
);

    function integer log2;
        input integer number;
        begin
            log2 = 0;
            while( 2**log2 < number)
            begin
              log2 = log2 + 1;
            end
        end
    endfunction // log2

/* ------------ Internal Params ------------ */
    localparam NUM_STATES            = 11;
    localparam CHECK_TCP_OQ_STATUS   = 1;
    localparam NDP_MODE_OFF          = 2;
    localparam NDP_MODE_ON           = 3;
    localparam LAST_WORD             = 4;
    localparam FLUSH_REST_PKT        = 5;
    localparam TCP_WORD              = 6;
    localparam NDP_MODE_ON_PIPE      = 7;
    localparam TCP_WORD_PIPE         = 8;
    localparam CHECK_ACK_NACK        = 9;
    localparam NDP_MODE_OFF_PIPE     = 10;
    localparam ACK_MODE_ON           = 11;

    localparam IP_PROTO              = 64;
    localparam TCP                   = 6;

    localparam NUM_QUEUES            = 5;

    // Per NetFPGA-10G AXI Spec
    localparam DST_POS        = 24;
    localparam DST_PQ_POS     = 32;
    localparam TCP_PROT_POS   = 23*8;
    localparam TCP_PROT       = 6;
    localparam NDP_PROT       = 199; //0xC7
    localparam IPHL_POS       = 136;
    localparam TL_POS         = 112;
    localparam TCPHL_POS      = 140;
    localparam IP_CKSM_POS    = 48;

    localparam ACK_POS        = 255-16-6;
    localparam NACK_POS       = 255-16-4;

/* ------------ Wires and Regs ------------ */

    wire [C_M_AXIS_DATA_WIDTH - 1:0]          tdata_fifo;
    wire [((C_M_AXIS_DATA_WIDTH / 8)) - 1:0]  tkeep_fifo;
    wire [C_S_AXIS_TUSER_WIDTH-1:0]           tuser_fifo;
    wire                                      tlast_fifo;

    wire [C_S_AXIS_DATA_WIDTH - 1:0]          s_axis_tdata_s;
    wire [((C_S_AXIS_DATA_WIDTH / 8)) - 1:0]  s_axis_tkeep_s;
    wire [C_S_AXIS_TUSER_WIDTH-1:0]           s_axis_tuser_s;
    wire                                      s_axis_tvalid_s;
    wire                                      s_axis_tready_s;
    wire                                      s_axis_tlast_s;

    reg [C_M_AXIS_DATA_WIDTH - 1:0]           m_axis_tdata_s;
    reg [((C_M_AXIS_DATA_WIDTH / 8)) - 1:0]   m_axis_tkeep_s;
    reg [C_M_AXIS_TUSER_WIDTH-1:0]            m_axis_tuser_s;
    reg                                       m_axis_tvalid_s;
    wire                                      m_axis_tready_s;
    reg                                       m_axis_tlast_s;

    reg      in_fifo_rd_en;
    wire     in_fifo_nearly_full;
    wire     in_fifo_empty;

    reg [NUM_STATES-1:0]   state, state_next;

    reg  [7:0]  tuser_HQ, tuser_PQ, tuser_HQ_next, tuser_PQ_next;
    reg  [7:0]  tuser_HQ_add, tuser_PQ_add;
    reg   [((C_S_AXIS_DATA_WIDTH / 8)) - 1:0]  tkeep, tkeep_next;

    wire [7:0]          dbg_tcp;
    wire [3+4:0]        dbg_tkeep;
    reg  [7:0]          dbg_tdata_fifo;

    reg [3:0]   iphl;             // internet header length
    reg [15:0]  total_length;     // entire packet size
    reg [3:0]   tcphl;            // tcp header length
    reg [15:0]  ipcksm;
    reg [31:0]  ipcksm_new;
    reg [255:0] tdata_chunk0;
    reg [255:0] tdata_chunk0_next;
    reg [127:0] tuser_chunk0;
    reg [127:0] tuser_chunk0_next;
    reg         tlast_chunk0;
    reg         tlast_chunk0_next;
    reg   [((C_S_AXIS_DATA_WIDTH / 8)) - 1:0]  tkeep_chunk0, tkeep_chunk0_next;


    reg [31:0]  ip_hdr_1, ip_hdr_2, ip_hdr_3, ip_hdr_4, ip_hdr_5;
    reg [31:0]  ip_hdr_1_next, ip_hdr_2_next, ip_hdr_3_next, ip_hdr_4_next, ip_hdr_5_next;
    reg [31:0]  pseudo_hdr, tcp_hdr_1,tcp_hdr_2,tcp_hdr_3, tcp_hdr_4, tcp_hdr_5;
    reg [31:0]  pseudo_hdr_next, tcp_hdr_1_next,tcp_hdr_2_next,tcp_hdr_3_next, tcp_hdr_4_next, tcp_hdr_5_next;


/* ------------ Modules and Logic ------------ */

   /* The size of this fifo has to be large enough to fit the previous modules' headers
    * and the ethernet header */
    fallthrough_small_fifo #(
        .WIDTH(C_M_AXIS_DATA_WIDTH+C_M_AXIS_TUSER_WIDTH+C_M_AXIS_DATA_WIDTH/8+1),
        .MAX_DEPTH_BITS(4)
    )
    input_fifo(
        .din         ({s_axis_tlast_s, s_axis_tuser_s, s_axis_tkeep_s, s_axis_tdata_s}),    // Data in
        .wr_en       (s_axis_tvalid_s & s_axis_tready_s),                                   // Write enable
        .rd_en       (in_fifo_rd_en),                                                       // Read the next word
        .dout        ({tlast_fifo, tuser_fifo, tkeep_fifo, tdata_fifo}),
        .full        (),
        .prog_full   (),
        .nearly_full (in_fifo_nearly_full),
        .empty       (in_fifo_empty),
        .reset       (~axis_resetn),
        .clk         (axis_aclk)
    );

    assign s_axis_tready_s = ~in_fifo_nearly_full;

    assign dbg_tkeep = tdata_fifo[143+4:140];
    assign dbg_tcp = tdata_fifo[TCP_PROT_POS+7: TCP_PROT_POS];

/*  so we have 256 bits (32 bytes) in each chunk of tdata_fifo,
    the ip header len is min 5 x 32-bits (20 bytes) to max
    15 x 32-bits (60 bytes); so ip header can be in max 2 chunks
    with the assumption of no options are in ip and tcp
    chunk0: metadata [6B]  eth [8B]  ip [18B]
    chunk1: ip       [2B]  tcp [20B] payload [10B]
                    208                       144
    [00 CA FE 00 00 01] [AA BB CC DD EE FF 08 00] [45 00 05 CE 00 01 00 00 40 00 F2 DC C0 A8 00 01 C0 A8
    01 01][00 14 00 50 00 00 00 00 00 00 00 00 50 02 20 00] [payload]

    FMS: Let's do nothing, up to some stuff in the main FIFO. Once something is there
    check AFULL signals from HQs and PQs to set the proper tuser and tuser_ext flags

    no backpreassure, we will ignore m_axis_tready signal */

    always @(*) begin
        m_axis_tuser_s  = tuser_fifo;
        state_next      = state;
        tkeep_next      = tkeep;
        m_axis_tvalid_s = 0;
        in_fifo_rd_en   = 0;
        m_axis_tlast_s  = tlast_fifo;
        m_axis_tkeep_s  = tkeep_fifo;
        m_axis_tdata_s  = tdata_fifo;
        dbg_tdata_fifo  = 8'h0;

        // pick up usefull header fields
        iphl          =  tdata_fifo[IPHL_POS+3:IPHL_POS];
        total_length  =  tdata_fifo[TL_POS+15:TL_POS];
        tcphl         =  tdata_fifo[TCPHL_POS+15:TCPHL_POS];
        ipcksm        =  tdata_fifo[IP_CKSM_POS+15:IP_CKSM_POS];
        ipcksm_new    =  0;

        tdata_chunk0_next   = tdata_chunk0;
        tkeep_chunk0_next   = tkeep_chunk0;
        tuser_chunk0_next   = tuser_chunk0;
        tlast_chunk0_next   = tlast_chunk0;

        ip_hdr_1_next = ip_hdr_1;
        ip_hdr_2_next = ip_hdr_2;
        ip_hdr_3_next = ip_hdr_3;
        ip_hdr_4_next = ip_hdr_4;
        ip_hdr_5_next = ip_hdr_5;

        tuser_HQ_next  = tuser_HQ;
        tuser_PQ_next  = tuser_PQ;

        case(state)

        CHECK_TCP_OQ_STATUS: begin

          tuser_HQ_next  = tuser_fifo[DST_POS+7:DST_POS];
          tuser_PQ_next  = 8'h00;

          if(~in_fifo_empty)
          begin
              dbg_tdata_fifo = tdata_fifo[IP_PROTO+7:IP_PROTO];

              m_axis_tvalid_s = 1;
              state_next      = NDP_MODE_OFF;
              in_fifo_rd_en   = 1;

              m_axis_tuser_s = tuser_fifo;

              if(tdata_fifo[IP_PROTO+7:IP_PROTO] == NDP_PROT)
              begin

                  m_axis_tvalid_s = 0;
                  tdata_chunk0_next     = tdata_fifo;
                  tuser_chunk0_next     = tuser_fifo;
                  tlast_chunk0_next     = tlast_fifo;
                  tkeep_chunk0_next     = tkeep_fifo;
                  state_next = CHECK_ACK_NACK;
              end
          end
      end

      CHECK_ACK_NACK: begin

      if(~in_fifo_empty)
      begin

          //((tuser_fifo[DST_PORT_POS+7:DST_PORT_POS]&oqs_status)!=8'b0)) begin
          // FIXME: this below should be re-think and simplified

          // by default we assume that ndp q is empty and we will send stuff there
          // but know we need one pipeline stage to fulsh first chunk0 then fifo

          m_axis_tvalid_s = 1;
          m_axis_tuser_s  = tuser_chunk0;
          m_axis_tdata_s  = tdata_chunk0;
          m_axis_tlast_s  = tlast_chunk0;
          m_axis_tkeep_s  = tkeep_chunk0;

          state_next      = NDP_MODE_OFF_PIPE;

	        if (tdata_fifo[ACK_POS] || tdata_fifo[NACK_POS] || (tdata_chunk0[TL_POS+15:TL_POS] ==16'h0028))  //FIXME: for simlicity we are checking header only pkts here,
          begin
              state_next        = ACK_MODE_ON;                                                          // this might be checked in the first FMS stage, also it tries to flush
              in_fifo_rd_en     = 0;                                                                    // data afterwards which adds another FMS state. Improvment needed.
              m_axis_tvalid_s   = 1;
              tuser_HQ_add = 8'h0;
              tuser_PQ_add = 8'h0;

    	      if (tuser_chunk0[DST_POS])
            begin
    		      tuser_PQ_add[0]  = 1'b1;
    	      end
    	      if (tuser_chunk0[DST_POS+2])
            begin
    		      tuser_PQ_add[2]  = 1'b1;
    	      end
    	      if (tuser_chunk0[DST_POS+4])
            begin
    		      tuser_PQ_add[4]  = 1'b1;
    	      end
    	      if (tuser_chunk0[DST_POS+6])
            begin
    		      tuser_PQ_add[6]  = 1'b1;
    	      end
    	      m_axis_tuser_s    = {tuser_chunk0[C_M_AXIS_TUSER_WIDTH-1:DST_PQ_POS+8], tuser_PQ_add, tuser_HQ_add, tuser_chunk0[DST_POS-1:0]};
	       end
         else
         begin
          if (tuser_chunk0[DST_POS]) begin
            if (H_0_full) begin
              tuser_HQ_next[0]  = 1'b0;
              tuser_PQ_next[0]  = 1'b1;
              state_next        = NDP_MODE_ON;
              in_fifo_rd_en     = 0;
              m_axis_tvalid_s   = 0;
            end
          end

          if (tuser_chunk0[DST_POS+2]) begin
            if (H_1_full) begin
              tuser_HQ_next[2]  = 1'b0;
              tuser_PQ_next[2]  = 1'b1;
              state_next = NDP_MODE_ON;
              in_fifo_rd_en     = 0;
              m_axis_tvalid_s   = 0;
            end
          end

          if (tuser_chunk0[DST_POS+4]) begin
            if (H_2_full) begin
              tuser_HQ_next[4]  = 1'b0;
              tuser_PQ_next[4]  = 1'b1;
              state_next = NDP_MODE_ON;
              in_fifo_rd_en       = 0;
              m_axis_tvalid_s     = 0;
            end
          end

          if (tuser_chunk0[DST_POS+6]) begin
            if (H_3_full) begin
              tuser_HQ_next[6]  = 1'b0;
              tuser_PQ_next[6]  = 1'b1;
              state_next = NDP_MODE_ON;
              in_fifo_rd_en         = 0;
              m_axis_tvalid_s       = 0;
            end
          end
          end
        end
      end

  ACK_MODE_ON: begin
  if(!in_fifo_empty) begin
    m_axis_tvalid_s = 1;
    in_fifo_rd_en = 1;
    if(tlast_fifo) begin
      m_axis_tlast_s = 1;
      state_next = CHECK_TCP_OQ_STATUS;
    end
  end
  end

  NDP_MODE_OFF_PIPE: begin

    m_axis_tvalid_s = 1;
    in_fifo_rd_en = 1;
    if(tlast_fifo)
      state_next   = CHECK_TCP_OQ_STATUS;
    else
      state_next   = NDP_MODE_OFF;
  end

  NDP_MODE_OFF: begin
    if(!in_fifo_empty)
    begin
      m_axis_tvalid_s = 1;
      in_fifo_rd_en = 1;
      if(m_axis_tlast_s)
      begin
        state_next = CHECK_TCP_OQ_STATUS;
      end
    end
  end

  NDP_MODE_ON: begin

      in_fifo_rd_en = 0;
      ip_hdr_1_next = tdata_chunk0[143:128] + 16'h0028            + tdata_chunk0[111:96];            //tdata_chunk0[127:112];
      ip_hdr_2_next = tdata_chunk0[95:80]   + tdata_chunk0[79:64] + tdata_chunk0[47:32];  // skip tuser_fifo[207: 192] => current crc
      ip_hdr_3_next = tdata_chunk0[31:16]   + tdata_chunk0[15:0]  + tdata_fifo[255:240];

      state_next = NDP_MODE_ON_PIPE;

  end

  NDP_MODE_ON_PIPE: begin

    // now we have chunk0 and a new tdata_fifo,
    // so calcuate: new length and new ip cksm
    // and send stuff on m_axis bus if posible

    ipcksm_new = ip_hdr_1_next + ip_hdr_2_next + ip_hdr_3_next;
    ipcksm_new[15:0] = ~(ipcksm_new[31:16] + ipcksm_new[15:0]);
    m_axis_tvalid_s   = 1;
    m_axis_tdata_s    = {tdata_chunk0[255:128], 16'h0028, tdata_chunk0[111:64], ipcksm_new[15:0], tdata_chunk0[47:0]};
    m_axis_tlast_s    = 0;
    m_axis_tuser_s    = {tuser_chunk0[C_M_AXIS_TUSER_WIDTH-1:DST_PQ_POS+8], tuser_PQ, tuser_HQ, tuser_chunk0[DST_POS-1:0]};
    state_next = TCP_WORD_PIPE;

  end

  TCP_WORD_PIPE: begin

    pseudo_hdr_next    = {8'h0, tdata_chunk0[IP_PROTO+7:IP_PROTO]} + tdata_chunk0[47:32] + tdata_chunk0[31:16] + tdata_chunk0[15:0] + tdata_fifo[255:240] + 16'h0014;
    tcp_hdr_1_next     = tdata_fifo[239:224] + tdata_fifo[223:208] + tdata_fifo[207:192] + tdata_fifo[191:176];
    tcp_hdr_3_next     = tdata_fifo[175:160] + tdata_fifo[159:144] + tdata_fifo[143:128] + tdata_fifo[127:112];
    tcp_hdr_5_next     = tdata_fifo[95:80];

    state_next = TCP_WORD;

  end

  TCP_WORD: begin

    //cksm for tcp part
    ipcksm_new    = pseudo_hdr + tcp_hdr_1 + tcp_hdr_3 + tcp_hdr_5;
    ipcksm_new[15:0] = ~(ipcksm_new[31:16] + ipcksm_new[15:0]);

    m_axis_tvalid_s = 1;
    m_axis_tlast_s  = 1;
    m_axis_tkeep_s  = 32'hfffffc00;

    // FIXME: currently chsum for ndp is not calcuated
    m_axis_tdata_s[127-16:112-16] =  16'h0000; //ipcksm_new[15:0];

    in_fifo_rd_en = 1;

    if(tlast_fifo)
        state_next = CHECK_TCP_OQ_STATUS;
    else begin
      state_next = FLUSH_REST_PKT;
    end
  end

  LAST_WORD: begin
    if(!in_fifo_empty) begin
      m_axis_tvalid_s = 1;
      in_fifo_rd_en   = 1;
      m_axis_tkeep_s  = tkeep;
      if(tlast_fifo)
        state_next = CHECK_TCP_OQ_STATUS;
      else begin
        m_axis_tlast_s = 1;
        state_next = FLUSH_REST_PKT;
      end
    end
  end

  FLUSH_REST_PKT: begin
    if(!in_fifo_empty) begin
      in_fifo_rd_en = 1;
      if(tlast_fifo)
        state_next = CHECK_TCP_OQ_STATUS;
    end
  end
  endcase // case(state)
end // always @ (*)

always @(posedge axis_aclk) begin
  if(~axis_resetn) begin
    state         <= CHECK_TCP_OQ_STATUS;
    tkeep         <= 32'hffffffff;
    tdata_chunk0  <= 256'h0;
    tuser_chunk0  <= 128'h0;
    tlast_chunk0  <= 1'b0;
    tkeep_chunk0  <= 32'hffffffff;
    tuser_HQ      <= 8'h0;
    tuser_PQ      <= 8'h0;

    ip_hdr_1      <= 8'h0;
    ip_hdr_2      <= 8'h0;
    ip_hdr_3      <= 8'h0;
    ip_hdr_4      <= 8'h0;
    ip_hdr_5      <= 8'h0;

    pseudo_hdr    <= 8'h0;
    tcp_hdr_1     <= 8'h0;
    tcp_hdr_2     <= 8'h0;
    tcp_hdr_3     <= 8'h0;
    tcp_hdr_4     <= 8'h0;
    tcp_hdr_5     <= 8'h0;

  end
  else begin
    state         <= state_next;
    tkeep         <= tkeep_next;
    tdata_chunk0  <= tdata_chunk0_next;
    tuser_chunk0  <= tuser_chunk0_next;
    tlast_chunk0  <= tlast_chunk0_next;
    tkeep_chunk0  <= tkeep_chunk0_next;
    tuser_HQ      <= tuser_HQ_next;
    tuser_PQ      <= tuser_PQ_next;

    ip_hdr_1      <= ip_hdr_1_next;
    ip_hdr_2      <= ip_hdr_2_next;
    ip_hdr_3      <= ip_hdr_3_next;
    ip_hdr_4      <= ip_hdr_4_next;
    ip_hdr_5      <= ip_hdr_5_next;

    pseudo_hdr    <= pseudo_hdr_next;
    tcp_hdr_1     <= tcp_hdr_1_next;
    tcp_hdr_2     <= tcp_hdr_2_next;
    tcp_hdr_3     <= tcp_hdr_3_next;
    tcp_hdr_4     <= tcp_hdr_4_next;
    tcp_hdr_5     <= tcp_hdr_5_next;
  end
end

bridge
  #(
     .C_AXIS_DATA_WIDTH (C_M_AXIS_DATA_WIDTH),
     .C_AXIS_TUSER_WIDTH (C_M_AXIS_TUSER_WIDTH)
   ) le_be_bridge
   (
   // Global Ports
    .clk(ACLK),
    .reset(~ARESETN),

   // little endian signals
   .s_axis_tready(s_axis_tready),
   .s_axis_tdata(s_axis_tdata),
   .s_axis_tlast(s_axis_tlast),
   .s_axis_tvalid(s_axis_tvalid),
   .s_axis_tuser(s_axis_tuser),
   .s_axis_tkeep(s_axis_tkeep),

   // big endian signals
   .m_axis_tready(s_axis_tready_s),
   .m_axis_tdata(s_axis_tdata_s),
   .m_axis_tlast(s_axis_tlast_s),
   .m_axis_tvalid(s_axis_tvalid_s),
   .m_axis_tuser(s_axis_tuser_s),
   .m_axis_tkeep(s_axis_tkeep_s)
   );

  bridge
  #(
     .C_AXIS_DATA_WIDTH (C_M_AXIS_DATA_WIDTH),
     .C_AXIS_TUSER_WIDTH (C_M_AXIS_TUSER_WIDTH)
   ) be_le_bridge
   (
   // Global Ports
    .clk(ACLK),
    .reset(~ARESETN),

   // little endian signals
   .s_axis_tready(m_axis_tready_s),
   .s_axis_tdata(m_axis_tdata_s),
   .s_axis_tlast(m_axis_tlast_s),
   .s_axis_tvalid(m_axis_tvalid_s),
   .s_axis_tuser(m_axis_tuser_s),
   .s_axis_tkeep(m_axis_tkeep_s),

   // big endian signals
   .m_axis_tready(m_axis_tready),
   .m_axis_tdata(m_axis_tdata),
   .m_axis_tlast(m_axis_tlast),
   .m_axis_tvalid(m_axis_tvalid),
   .m_axis_tuser(m_axis_tuser),
   .m_axis_tkeep(m_axis_tkeep)
   );


endmodule
