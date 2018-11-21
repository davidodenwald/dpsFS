#include "fstream"
#include "vector"
#include "iostream"

using namespace std;

int main(void) {
  vector<unsigned short> ints;

  ints.push_back(8);
  ints.push_back(16);
  ints.push_back(32);
  ints.push_back(64);
  ints.push_back(128);
  ints.push_back(61440);

  ofstream out;
  out.open("ints"); 

  for(int i = 0; i < ints.size(); i++) {
    out.write((char *)&ints[i], 2);
  }
  out.close();

  ifstream in;
  in.open("ints");
  unsigned short x;

  for(int i = 0; i < ints.size(); i++) {
    in.read((char *)&x, 2);
    cout << x << endl;
  }
  in.close();

  return 0;
}