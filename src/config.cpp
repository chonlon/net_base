#include "config.h"
#include "base/string_piece.h"

namespace lon {


const void* JsonConfig::getImpl(const String& key) const {
	if (auto iter = miss_map_.find(key); iter != miss_map_.end()) return &(*iter);
}

const void* YamlConfig::getImpl(const String& key) const {
	if(auto iter = miss_map_.find(key); iter!= miss_map_.end()) return &(*iter);
	auto keys = splitStringPiece(key, ".");
    const YAML::Node current = node_;
	for(auto& key : keys) {
	    if(!current) {
	        return nullptr;
	    }

		current = current[key];
	}
	return &(current.as<int>());
}

}
