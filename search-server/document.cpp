#include "document.h"


std::ostream& operator<<(std::ostream& out, const Document& iter) {

    using namespace std;
    out << "{ document_id = "s << iter.id << ", relevance = "s << iter.relevance << ", rating = "s << iter.rating << " }"s;
    return out;

}

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}