#pragma once
#include <vector>
#include <cassert>
#include <ostream>

template<typename Iterator>
class IteratorRange {

public:
    IteratorRange(Iterator begin_page, Iterator end_page)
        : page_begin_(begin_page), page_end_(end_page)
    {
    }

    auto begin() const { return page_begin_; }
    auto end() const { return page_end_; }

private:
    Iterator page_begin_;
    Iterator page_end_;
};

template<typename Iterator>
class Paginator {

public:
    Paginator(Iterator first, Iterator last, int page_size) {
        
        assert(last >= first && page_size > 0);

        int all_docs_amount = distance(first, last);
        int fully_filled_pages_amount = all_docs_amount / page_size;
        int last_page_size = all_docs_amount % page_size;

        for (int i = 0; i < fully_filled_pages_amount; ++i) {
            Iterator page_begin = next(first, page_size * i);
            Iterator page_end = next(page_begin, page_size - 1);
            auto page = IteratorRange<Iterator>(page_begin, page_end);
            pages_.push_back(page);
        }
        if (last_page_size != 0) {
            Iterator last_page_begin = next(first, page_size * fully_filled_pages_amount);
            Iterator last_page_end = prev(last, 1);
            auto page = IteratorRange<Iterator>(last_page_begin, last_page_end);
            pages_.push_back(page);
        }
    }

    auto begin() const { return pages_.begin(); }
    auto end() const { return pages_.end(); }
    auto size() const { return pages_.size(); }

private:

    std::vector<IteratorRange<Iterator>> pages_;

};


template <typename Container>
auto Paginate(const Container& c, size_t page_size);

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

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
