#include <string>
#include <vector>

#include "mylog.h"
using namespace std;

vector<string> get_ev_mode(std::vector<std::pair<std::string, int>> *mapEV);
int get_real_index_with_ev_map(int index, vector<string> *selected_ev_mode,
                               std::vector<std::pair<std::string, int>> *mapEV);