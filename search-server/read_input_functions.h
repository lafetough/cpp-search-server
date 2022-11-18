#pragma once
#include <ostream>
#include "document.h"
#include "paginator.h"

std::ostream& operator<<(std::ostream& out, const Document& iter);

template<typename Iterator>
std::ostream& operator<<(std::ostream& o, IteratorRange<Iterator> iter);

template<typename Iterator>
std::ostream& operator<<(std::ostream& o, IteratorRange<Iterator> iter) {

    if (iter.begin() == iter.end()) {
        o << *iter.begin();
        return o;
    }
    o << *iter.begin() << *iter.end();
    return o;

}