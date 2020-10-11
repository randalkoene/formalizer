// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <cstring>

// core
#include "error.hpp"
#include "Graphshared.hpp"


namespace fz {

// temporarily out to prevent library make problems
#ifdef IS_READY_TO_COMPILE

Node_ID_shr::Node_ID_shr(const Node_ID & nid): idkey(nid.key()) {
    std::strncpy(idS_cache, nid.str().c_str(), NODE_ID_STRSZ); // automatically pads zeros
}

Edge_ID_shr::Edge_ID_shr(const Edge_ID & eid): idkey(eid.key()) {
    std::strncpy(idS_cache, eid.str().c_str(), EDGE_ID_STRSZ); // automatically pads zeros
}

Topic_Keyword_shr::Topic_Keyword_shr(const Topic_Keyword & tkey): relevance(tkey.relevance) {
    std::strncpy(keyword, tkey.keyword.c_str(), TOPIC_KEYWORD_STRSZ);
}

Topic_shr::Topic_shr(const Topic & topic): id(topic.get_id()), supid(topic.get_supid()), keyrelnum(0) {
    std::strncpy(tag, topic.get_tag().c_str(), TOPIC_TAG_STRSZ);
    std::strncpy(title, topic.get_title().c_str(), TOPIC_TITLE_STRSZ);
    if (topic.get_keyrel().size() > TOPIC_KEYREL_ARRSZ) {
        ADDERROR(__func__, "Too many Topic keywords. TOPIC_KEYREL_ARRSZ needs to be at least "+std::to_string(topic.get_keyrel().size()));
        throw std::length_error("Too many Topic keywords");

    } else {
        for (size_t i = 0; i < topic.get_keyrel().size(); ++i) {
            keyrel[i] = topic.get_keyrel()[i];
            ++keyrelnum;
        }
    }
}

#endif // IS_READY_TO_COMPILE

} // namespace fz
