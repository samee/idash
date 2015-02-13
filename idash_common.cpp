#include<fstream>
#include<iostream>
#include<string>
#include<unordered_map>
using namespace std;

// ----------------- C++ standard can't hash pairs -----------------------
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
  template<typename S, typename T> struct hash<pair<S, T>>
  {
    inline size_t operator()(const pair<S, T> & v) const
    {
      size_t seed = 0;
      ::hash_combine(seed, v.first);
      ::hash_combine(seed, v.second);
      return seed;
    }
  };
}
// ------------------- pair hashing support ends ------------------------
void insertValidPositions(unordered_map<string,int>& posmap,const char* file)
{
  ifstream in(file);
  string line;
  if(!in) { cerr<<"Couldn't open sequence csv file "<<file<<endl; throw -1; }
  while(getline(in,line)) posmap.insert(make_pair(line,posmap.size()+1));
}
typedef uint8_t byte;
void insertValidPositions(
    unordered_map<pair<byte,int>,int>& posmap,
    const char* file)
{
  ifstream in(file);
  long long chrom,pos;
  char comma;
  if(!in) { cerr<<"Couldn't open sequence csv file "<<file<<endl; throw -1; }
  while(in>>chrom>>comma>>pos)
    posmap.insert(make_pair(make_pair(chrom,pos),posmap.size()+1));
}

int main()
{
  try{
    // Micro-optimization: changing type to string reduces loading time from
    //   4.7 to 4.4 seconds, at least when loading from HDD.
    unordered_map<string,int> seq;
    //unordered_map<pair<byte,int>,int> seq;
    insertValidPositions(seq,"seq1.CSV");
    insertValidPositions(seq,"seq2.CSV");
//    for(const auto &kv:seq) cout<<kv.first<<" --> "<<kv.second<<'\n';
//    for(const auto &kv:seq) cout<<int(kv.first.first)<<','<<kv.first.second
//                                <<" --> "<<kv.second<<'\n';
  } catch (int x) { return x; }
}
