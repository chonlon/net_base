#include "config.h"
#include "base/lstring.h"
#include <set>
#include <yaml-cpp/node/node.h>




namespace lon {

detail::YamlConfigLoader::YamlConfigLoader(String str) : config{str}{
}

detail::JsonConfigLoader::JsonConfigLoader(String str) : config{str} {

}
}
