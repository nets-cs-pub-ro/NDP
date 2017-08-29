#include "connection_matrix.h"
#include <string.h>
#include <iostream>

ConnectionMatrix::ConnectionMatrix(int n)
{
  N = n;
}

void ConnectionMatrix::setPermutation(int conn){
  setPermutation(conn,1);
}

void ConnectionMatrix::addConnection(int src, int dest){
  if (connections.find(src)==connections.end())
    connections[src] = new vector<int>();
  
  connections[src]->push_back(dest);
}

void ConnectionMatrix::setPermutation(int conn, int rack_size){
  int is_dest[N],dest,pos;
  int to[N];
  int* rack_load,rack_count;
  vector<int> perm_tmp;
  int i;

  rack_count = N/rack_size;
  rack_load = (int*)calloc(rack_count,sizeof(int));
  
  for (i=0;i<N;i++){
    is_dest[i] = 0;
    to[N] = -1;
    perm_tmp.push_back(i);
  }

  for (int src = 0; src < N; src++) {
    do {
      pos = rand()%perm_tmp.size();
    } while(src==perm_tmp[pos]&&perm_tmp.size()>1);

    dest = perm_tmp[pos];
    assert(src!=dest);

    perm_tmp.erase(perm_tmp.begin()+pos);
    to[src] = dest;
  }

  for (i = 0;i<conn;i++){
    if (!perm_tmp.size())
      for (int q=0;q<N;q++){
	perm_tmp.push_back(q);
      }

    pos = rand()%perm_tmp.size();

    if (rack_count<N){
      //int pos2 = rand()%perm_tmp.size();

      //if (rack_load[perm_tmp[pos]/rack_size]>rack_load[perm_tmp[pos2]/rack_size])
      //pos = pos2;
    }

    int src = perm_tmp[pos];
    printf("src=%d rack_size=%d\n", src, rack_size);
    rack_load[src/rack_size]++;
    perm_tmp.erase(perm_tmp.begin()+pos);

    if (connections.find(src)==connections.end()){
      connections[src] = new vector<int>();
    }
    connections[src]->push_back(to[src]);
  }

  cout << "Rack load: ";
  for (i=0;i<rack_count;i++)
    cout << rack_load[i] << " ";

  cout <<endl;

  free(rack_load);
}

void ConnectionMatrix::setPermutation(){
  int is_dest[N],dest;
  
  for (int i=0;i<N;i++)
    is_dest[i] = 0;

  for (int src = 0; src < N; src++) {
    vector<int>* destinations = new vector<int>();
      
    int r = rand()%(N-src);
    for (dest = 0;dest<N;dest++){
      if (r==0&&!is_dest[dest])
	break;
      if (!is_dest[dest])
	r--;
    }

    if (r!=0||is_dest[dest]){
      cout << "Wrong connections r " <<  r << "is_dest "<<is_dest[dest]<<endl;
      exit(1);
    }
      
    if (src==dest){
      //find first other destination that is different!
      do {
	dest = (dest+1)%N;
      }
      while (is_dest[dest]);
	
      if (src==dest){
	printf("Wrong connections 2!\n");
	exit(1);
      }
    }
    is_dest[dest] = 1;
    destinations->push_back(dest);

    connections[src] = destinations;    
  }
}

void ConnectionMatrix::setStride(int S){
  for (int src = 0;src<N; src++) {
    int dest = (src+S)%N;

    connections[src] = new vector<int>();
    connections[src]->push_back(dest);
  }
}

void ConnectionMatrix::setLocalTraffic(Topology* top){
  for (int src = 0;src<N;src++){
    connections[src] = new vector<int>();
    vector<int>* neighbours = top->get_neighbours(src);
    for (unsigned int n=0;n<neighbours->size();n++)
      connections[src]->push_back(neighbours->at(n));
  }
}

void ConnectionMatrix::setRandom(int cnx){
  for (int conn = 0;conn<cnx; conn++) {
    int src = rand()%N;
    int dest = rand()%N;

    if (src==0||dest==N-1){
      conn--;
      continue;
    }      

    if (connections.find(src)==connections.end()){
      connections[src] = new vector<int>();
    }

    connections[src]->push_back(dest);
  }
}

void ConnectionMatrix::setVL2(){
  for (int src = 0;src<N; src++) {
    connections[src] = new vector<int>();

    //need to draw a number from VL2 distribution
    int crt = -1;
    double coin = drand();
    if (coin<0.3)
      crt = 1;
    else if (coin<0.35)
      crt = 1+rand()%10;
    else if (coin<0.85)
      crt = 10;
    else if (coin<0.95)
      crt = 10+rand()%70;
    else 
      crt = 80;

    for (int i = 0;i<crt;i++){
      int dest = rand()%N;
      if (src==dest){
	i--;
	continue;
      }
      connections[src]->push_back(dest);
    }
  }
}

vector<connection*>* ConnectionMatrix::getAllConnections(){
  vector<connection*>* ret = new vector<connection*>();
  vector<int>* destinations;
  map<int,vector<int>*>::iterator it;

  for (it = connections.begin(); it!=connections.end();it++){
    int src = (*it).first;
    destinations = (vector<int>*)(*it).second;
    
    vector<int> subflows_chosen;
    
    for (unsigned int dst_id = 0;dst_id<destinations->size();dst_id++){
      connection* tmp = new connection();
      tmp->src = src;
      tmp->dst = destinations->at(dst_id);
      ret->push_back(tmp);
    }
  }
  return ret;
}

void ConnectionMatrix::setStaggeredRandom(Topology* top,int conns,double local){
  for (int conn = 0;conn<conns; conn++) {
    int src = rand()%N;

    if (connections.find(src)==connections.end()){
      connections[src] = new vector<int>();
    }

    vector<int>* neighbours = top->get_neighbours(src);

    int dest;
    if (drand()<local){
      dest = neighbours->at(rand()%neighbours->size());
    }
    else {
      dest = rand()%N;
    }
    connections[src]->push_back(dest);
  }
}

void ConnectionMatrix::setStaggeredPermutation(Topology* top,double local){
  int is_dest[N];
  int dest = -1,i,found = 0;

  memset(is_dest,0,N*sizeof(int));

  for (int src = 0;src<N; src++) {
    connections[src] = new vector<int>();
    vector<int>* neighbours = top->get_neighbours(src);

    double v = drand();
    if (v<local){
      i = 0;
      do {
	found = 0;
	dest = neighbours->at(rand()%neighbours->size());
	if (is_dest[dest])
	  found = 1;
      }
      while (found && i++<15);
    }
    
    if (v>=local || (v<local&&found)){
      dest = rand()%N;
      while (is_dest[dest])
	dest = (dest+1)%N;
    }

    assert(dest>=0&&dest<N);
    assert(is_dest[dest]==0);
     
    connections[src]->push_back(dest);
    is_dest[dest] = 1;
  }
}

void ConnectionMatrix::setManytoMany(int c){
  vector<int> hosts;
  int t,f;
  unsigned int i,j;

  for (i=0;i<(unsigned int)c;i++){
    do {
      t = rand()%N;
      f = 0;
      for (j=0;j<hosts.size();j++)
	if (hosts[j]==t){
	  f = 1;
	  break;
	}
    }while(f);

    hosts.push_back(t);
  }

  for (i=0;i<hosts.size();i++){
    connections[hosts[i]] = new vector<int>();
    for (j=0;j<hosts.size();j++){
      if (i==j)
	continue;
      connections[hosts[i]]->push_back(hosts[j]);
    }
  }
}

void ConnectionMatrix::setHotspot(int hosts_per_hotspot, int count){
  int is_dest[N],is_done[N];
  for (int i=0;i<N;i++){
    is_dest[i] = 0;
    is_done[i] = 0;
  }

  int first, src;
  for (int k=0;k<count;k++){
    do {
      first = rand()%N;
    }
    while (is_dest[first]);
    
    is_dest[first] = 1;
    
    for (int i=0;i<hosts_per_hotspot;i++){
      do{
	if (hosts_per_hotspot==N)
	  src = i;
	else
	  src = rand()%N;
      }
      while(is_done[src]);
      is_done[src]=1;

      if (connections.find(src)==connections.end())      
	connections[src] = new vector<int>();

      connections[src]->push_back(first);
      is_done[src] = 1;
    }  
  }
}

void ConnectionMatrix::setIncast(int hosts_per_hotspot, int center){
  int is_dest[N],is_done[N];
  for (int i=0;i<N;i++){
    is_dest[i] = 0;
    is_done[i] = 0;
  }

  // creates an incast to destination 0
  int first = 0;

  for (int i=center;i<hosts_per_hotspot+center;i++){
    if (connections.find(i)==connections.end())      
      connections[i] = new vector<int>();
    
    connections[i]->push_back(first);
  }  
}

void ConnectionMatrix::setOutcast(int hosts_per_hotspot, int center){
  int is_dest[N],is_done[N];
  for (int i=0;i<N;i++){
    is_dest[i] = 0;
    is_done[i] = 0;
  }

  // creates an outcast from source 0 to destinations center to centre+hosts_per_hotspot
  int src = 0;
  connections[src] = new vector<int>();

  for (int i=center;i<hosts_per_hotspot+center;i++){
    connections[src]->push_back(i);
  }  
}

/*
void ConnectionMatrix::setHotspot(int hosts){
  int is_dest[N],dest,is_done[N];
  for (int i=0;i<N;i++){
    is_dest[i] = 0;
    is_done[i] = 0;
  }

  int first = rand()%N;
  is_done[first] = 1;
  is_dest[first] = 1;

  for (int i=0;i<hosts-1;i++){
    int src;
    do{
      src = rand()%N;
    }
    while(is_done[src]);
    is_done[src]=1;
    connections[src] = new vector<int>();
    connections[src]->push_back(first);
    //is_dest[src] = 1;
  }  

  for (int src = 0; src < N; src++) {
    if (is_done[src])
      continue;

    vector<int>* destinations = new vector<int>();
      
    int r = rand()%(N-src);
    for (dest = 0;dest<N;dest++){
      if (r==0&&!is_dest[dest])
	break;
      if (!is_dest[dest])
	r--;
    }

    if (r!=0||is_dest[dest]){
      cout << "Wrong connections r " <<  r << "is_dest "<<is_dest[dest]<<endl;
      exit(1);
    }
      
    if (src==dest){
      //find first other destination that is different!
      do {
	dest = (dest+1)%N;
      }
      while (is_dest[dest]);
	
      if (src==dest){
	printf("Wrong connections 2!\n");
	exit(1);
      }
    }
    is_dest[dest] = 1;
    destinations->push_back(dest);

    connections[src] = destinations;    
  }
}
*/

