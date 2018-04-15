//
// Source: http://h-deb.clg.qc.ca/Sources/ntuple_buffer.html
// Auteur: Patrice Roy
//

#ifndef SODIUM_TRACKER_DOUBLETAMPON_H
#define SODIUM_TRACKER_DOUBLETAMPON_H

#include <iostream>
#include <fstream>
#include <string>
#include <atomic>
#include <future>
#include <vector>
#include <algorithm>
#include <utility>
#include <chrono>
#include <iterator>
#include <array>
using namespace std;
using namespace std::chrono;

template <class, int, int>
class ntuple_buffer;
template <class T, int N>
using double_buffer = ntuple_buffer<T, 2, N>;

template <class T, int NBUFS, int SZBUF>
class ntuple_buffer
{
    using this_type = ntuple_buffer<T, NBUFS, SZBUF>;
    array<array<T,SZBUF>,NBUFS> bufs;
public:
    using bufndx_t = typename array<array<T,SZBUF>,NBUFS>::size_type;
    using ndx_t = typename array<T,SZBUF>::size_type;
    static bufndx_t next_buffer(bufndx_t n) { return n == NBUFS - 1? 0 : n + 1; }
    static bufndx_t prev_buffer(bufndx_t n) { return n? n - 1 : NBUFS - 1; }
private:
    atomic<bufndx_t> read_buf, write_buf;
    atomic<ndx_t> cur_write;
public:
    ntuple_buffer()
            : read_buf{0}, write_buf{0}, cur_write{0}
    {
    }
    template <class C>
    void ajouter(const C &src)
    {
        ajouter(begin(src), end(src));
    }
    template <class It>
    void ajouter(It debut, It fin)
    {
        while (debut != fin)
        {
            auto n = distance(debut, fin);
            auto m = bufs[write_buf].size() - cur_write;
            auto ntoadd = min<decltype(n)>(m,n);
            copy(debut, debut + ntoadd, begin(bufs[write_buf]) + cur_write);
            cur_write += ntoadd;
            advance(debut, ntoadd);
            if (cur_write == bufs[write_buf].size())
            {
                write_buf = next_buffer(write_buf);
                cur_write = {};
            }
        }
    }
    vector<T> extraire()
    {
        vector<T> resultat;
        while(read_buf != write_buf) // ICI: ambitieux
        {
            resultat.insert(end(resultat), begin(bufs[read_buf]), end(bufs[read_buf]));
            read_buf = next_buffer(read_buf);
        }
        return resultat;
    }
    vector<T> extraire_tout()
    {
        auto resultat = extraire();
        resultat.insert(end(resultat), begin(bufs[write_buf]), begin(bufs[write_buf]) + cur_write);
        return resultat;
    }
    template <class C>
    C extraire() volatile
    {
        auto res = extraire();
        return C{begin(res), end(res)};
    }
    template <class C>
    C extraire_tout() volatile
    {
        auto res = extraire_tout();
        return C{begin(res), end(res)};
    }
    template <class C>
    void ajouter(const C &src) volatile
    {
        const_cast<this_type&>(*this).ajouter(src);
    }
    template <class It>
    void ajouter(It debut, It fin) volatile
    {
        const_cast<this_type&>(*this).ajouter(debut, fin);
    }
    vector<T> extraire() volatile
    {
        return const_cast<this_type&>(*this).extraire();
    }
    vector<T> extraire_tout() volatile
    {
        return const_cast<this_type&>(*this).extraire_tout();
    }
};

#endif //SODIUM_TRACKER_DOUBLETAMPON_H
