#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

static const int32_t SENTINEL = INT32_MAX;

struct Node { int32_t key; Node* left; Node* right; };

class PtrBST {
    std::vector<Node> pool;
    int nextNode = 0;
    Node* buildRec(const std::vector<int32_t>& s, int lo, int hi) {
        if (lo > hi) return nullptr;
        int mid = lo + (hi - lo) / 2;
        Node* n = &pool[nextNode++];
        n->key   = s[mid];
        n->left  = buildRec(s, lo, mid-1);
        n->right = buildRec(s, mid+1, hi);
        return n;
    }
public:
    Node* root = nullptr;
    void build(const std::vector<int32_t>& s) {
        nextNode = 0;
        pool.resize(s.size());
        root = buildRec(s, 0, (int)s.size()-1);
    }
    bool search(int32_t key) const {
        Node* cur = root;
        while (cur) {
            if (key == cur->key) return true;
            cur = (key < cur->key) ? cur->left : cur->right;
        }
        return false;
    }
};

class EytzingerLayout {
    std::vector<int32_t> tree;
    void fill(const std::vector<int32_t>& s, int lo, int hi, int pos) {
        if (lo > hi || pos >= (int)tree.size()) return;
        int mid = lo + (hi - lo) / 2;
        tree[pos] = s[mid];
        fill(s, lo, mid-1, 2*pos);
        fill(s, mid+1, hi, 2*pos+1);
    }
public:
    void build(const std::vector<int32_t>& s) {
        int sz = 1;
        while (sz <= (int)s.size()) sz <<= 1;
        tree.assign(sz+1, SENTINEL);
        fill(s, 0, (int)s.size()-1, 1);
    }
    bool search(int32_t key) const {
        int i = 1;
        while (i < (int)tree.size() && tree[i] != SENTINEL) {
            if (tree[i] == key) return true;
            i = (key < tree[i]) ? 2*i : 2*i+1;
        }
        return false;
    }
};

class VebLayout {
    std::vector<int32_t> layout;
    int height = 0;

    static int treeSize(int h) { return (1 << h) - 1; }

    void fillBfs(const std::vector<int32_t>& s,
                 std::vector<int32_t>& bfs,
                 int node, int lo, int hi) {
        if (lo > hi || node >= (int)bfs.size()) return;
        int mid = lo + (hi - lo) / 2;
        bfs[node] = s[mid];
        fillBfs(s, bfs, 2*node,   lo,    mid-1);
        fillBfs(s, bfs, 2*node+1, mid+1, hi);
    }

    void toVeb(const std::vector<int32_t>& bfs,
               int root, int h, int& pos) {
        if (h == 0) return;
        if (h == 1) {
            layout[pos++] = (root < (int)bfs.size()) ? bfs[root] : SENTINEL;
            return;
        }
        int ht = (h+1)/2;   // ceil(h/2)
        int hb = h/2;       // floor(h/2)
        int num_bot = 1 << ht;

        toVeb(bfs, root, ht, pos);

        int first = root * num_bot;
        for (int i = 0; i < num_bot; i++)
            toVeb(bfs, first + i, hb, pos);
    }

    bool searchVeb(int32_t key, int& offset, int h) const {
        if (h == 0) return false;
        if (h == 1) {
            bool found = (layout[offset] == key);
            offset++;
            return found;
        }
        int ht = (h+1)/2;
        int hb = h/2;
        int top_sz  = treeSize(ht);
        int bot_sz  = treeSize(hb);
        int num_bot = 1 << ht;

        int top_start = offset;
        int bot_idx = 0;
        bool found = navTop(key, offset, ht, bot_idx);

        offset = top_start + top_sz;

        if (found) {
            offset += num_bot * bot_sz;
            return true;
        }
        if (bot_idx < 0 || bot_idx >= num_bot) {
            offset += num_bot * bot_sz;
            return false;
        }

        offset += bot_idx * bot_sz;
        bool res = searchVeb(key, offset, hb);
        offset += (num_bot - bot_idx - 1) * bot_sz;
        return res;
    }

    bool navTop(int32_t key, int& start, int h, int& bot_idx) const {
        if (h == 0) { return false; }

        if (h == 1) {
            int32_t k = layout[start++];
            if (k == SENTINEL || k == key) {
                if (k == key) return true;
                bot_idx = 0;
                return false;
            }
            bot_idx = (key < k) ? 0 : 1;
            return false;
        }

        int ht2 = (h+1)/2;
        int hb2 = h/2;
        int top2_sz = treeSize(ht2);
        int bot2_sz = treeSize(hb2);
        int num_bot2 = 1 << ht2;
        int bots_per = 1 << hb2;

        int save = start;

        int inner_bot = 0;
        bool found = navTop(key, start, ht2, inner_bot);
        start = save + top2_sz;

        if (found) {
            start += num_bot2 * bot2_sz;
            return true;
        }

        if (inner_bot < 0 || inner_bot >= num_bot2) {
            start += num_bot2 * bot2_sz;
            bot_idx = (inner_bot < 0) ? 0 : (1 << h);
            return false;
        }

        start += inner_bot * bot2_sz;
        int local_bot = 0;
        found = navTop(key, start, hb2, local_bot);
        start += (num_bot2 - inner_bot - 1) * bot2_sz;

        if (found) return true;

        bot_idx = inner_bot * bots_per + local_bot;
        return false;
    }

public:
    void build(const std::vector<int32_t>& s) {
        int n = (int)s.size();
        height = 1;
        while (treeSize(height) < n) height++;
        int sz = treeSize(height);

        std::vector<int32_t> bfs(sz+1, SENTINEL);
        fillBfs(s, bfs, 1, 0, n-1);

        layout.resize(sz);
        int pos = 0;
        toVeb(bfs, 1, height, pos);
    }

    bool search(int32_t key) const {
        int offset = 0;
        return searchVeb(key, offset, height);
    }
};

using Clk = std::chrono::high_resolution_clock;
double ms(Clk::time_point a, Clk::time_point b) {
    return std::chrono::duration<double,std::milli>(b-a).count();
}

int main(int argc, char* argv[]) {
    int N = 1 << 20;
    int Q = 1 << 20;
    int T = 5;

    if (argc > 1) N = std::stoi(argv[1]);
    if (argc > 2) Q = std::stoi(argv[2]);
    if (argc > 3) T = std::stoi(argv[3]);

    std::cout << "========================================================\n";
    std::cout << "  Benchmark de busqueda: BST de punteros, Eytzinger y vEB\n";
    std::cout << "========================================================\n";
    std::cout << "N=" << N << "  Q=" << Q << "  T=" << T << "\n\n";

    std::vector<int32_t> sorted(N);
    std::iota(sorted.begin(), sorted.end(), 1);

    std::mt19937 rng(42);
    std::vector<int32_t> queries(Q);
    std::uniform_int_distribution<int32_t> qd(1, 2*N);
    for (auto& q : queries) q = qd(rng);

    std::cout << "Armando estructuras...\n";

    auto t0 = Clk::now();
    PtrBST bst; bst.build(sorted);
    std::cout << "  BST de punteros : " << std::fixed << std::setprecision(1) << ms(t0,Clk::now()) << " ms\n";

    t0 = Clk::now();
    EytzingerLayout eyt; eyt.build(sorted);
    std::cout << "  Eytzinger       : " << ms(t0,Clk::now()) << " ms\n";

    t0 = Clk::now();
    VebLayout veb; veb.build(sorted);
    std::cout << "  Layout vEB      : " << ms(t0,Clk::now()) << " ms\n\n";

    std::cout << "Chequeando resultados... ";
    bool ok = true;
    std::uniform_int_distribution<int32_t> verifyDist(1, N);
    for (int i = 0; i < 1000 && ok; i++) {
        int32_t k = verifyDist(rng);
        bool b=bst.search(k), e=eyt.search(k), v=veb.search(k);
        if (b!=e||b!=v){std::cerr<<"MISMATCH hit k="<<k<<" bst="<<b<<" eyt="<<e<<" veb="<<v<<"\n"; ok=false;}
    }
    for (int i = 0; i < 1000 && ok; i++) {
        int32_t k = N + verifyDist(rng);
        bool b=bst.search(k), e=eyt.search(k), v=veb.search(k);
        if (b!=e||b!=v){std::cerr<<"MISMATCH miss k="<<k<<" bst="<<b<<" eyt="<<e<<" veb="<<v<<"\n"; ok=false;}
    }
    std::cout << (ok ? "OK\n\n" : "FAIL\n\n");
    if (!ok) return 1;

    std::vector<double> bstTimes(T), eytTimes(T), vebTimes(T);
    long long bstHits=0, eytHits=0, vebHits=0;

    std::cout << "Corriendo " << T << " rondas de " << Q << " queries...\n";
    std::string line(65,'-');
    std::cout << line << "\n";
    std::cout << std::setw(4)<<"Run"<<std::setw(14)<<"BST(ms)"
              <<std::setw(16)<<"Eytzinger(ms)"<<std::setw(12)<<"vEB(ms)"
              <<std::setw(10)<<"Sp(Eyt)"<<std::setw(10)<<"Sp(vEB)\n";
    std::cout << line << "\n";

    for (int t = 0; t < T; t++) {
        long long b=0,e=0,v=0;
        auto a=Clk::now();
        for(int q=0;q<Q;q++) if(bst.search(queries[q])) b++;
        auto bb=Clk::now(); bstTimes[t]=ms(a,bb);

        a=Clk::now();
        for(int q=0;q<Q;q++) if(eyt.search(queries[q])) e++;
        bb=Clk::now(); eytTimes[t]=ms(a,bb);

        a=Clk::now();
        for(int q=0;q<Q;q++) if(veb.search(queries[q])) v++;
        bb=Clk::now(); vebTimes[t]=ms(a,bb);

        bstHits+=b; eytHits+=e; vebHits+=v;
        std::cout<<std::setw(4)<<(t+1)
                 <<std::setw(14)<<std::fixed<<std::setprecision(2)<<bstTimes[t]
                 <<std::setw(16)<<eytTimes[t]<<std::setw(12)<<vebTimes[t]
                 <<std::setw(9)<<std::setprecision(3)<<bstTimes[t]/eytTimes[t]<<"x"
                 <<std::setw(9)<<bstTimes[t]/vebTimes[t]<<"x\n";
    }

    auto avg=[](std::vector<double>&v){return std::accumulate(v.begin(),v.end(),0.0)/v.size();};
    auto mn=[](std::vector<double>&v){return *std::min_element(v.begin(),v.end());};
    double ba=avg(bstTimes),ea=avg(eytTimes),va=avg(vebTimes);
    double bm=mn(bstTimes),em=mn(eytTimes),vm=mn(vebTimes);

    std::cout<<line<<"\n";
    std::cout<<std::setw(4)<<"AVG"<<std::setw(14)<<std::fixed<<std::setprecision(2)<<ba
             <<std::setw(16)<<ea<<std::setw(12)<<va
             <<std::setw(9)<<std::setprecision(3)<<ba/ea<<"x"<<std::setw(9)<<ba/va<<"x\n";
    std::cout<<std::setw(4)<<"MIN"<<std::setw(14)<<bm<<std::setw(16)<<em<<std::setw(12)<<vm
             <<std::setw(9)<<bm/em<<"x"<<std::setw(9)<<bm/vm<<"x\n";
    std::cout<<line<<"\n\n";
    std::cout<<"Hits: BST="<<bstHits<<"  Eyt="<<eytHits<<"  vEB="<<vebHits<<"\n";

    std::ofstream csv("/home/claude/cache_oblivious/results.csv");
    csv<<"exp,N,Q,bst_ms,eyt_ms,veb_ms,sp_eyt,sp_veb\n";
    for(int t=0;t<T;t++)
        csv<<(t+1)<<","<<N<<","<<Q<<","
           <<std::fixed<<std::setprecision(3)
           <<bstTimes[t]<<","<<eytTimes[t]<<","<<vebTimes[t]<<"," 
           <<bstTimes[t]/eytTimes[t]<<","<<bstTimes[t]/vebTimes[t]<<"\n";
    csv<<"AVG,"<<N<<","<<Q<<","<<ba<<","<<ea<<","<<va<<","<<ba/ea<<","<<ba/va<<"\n";

    std::ofstream js("/home/claude/cache_oblivious/results.json");
    js<<"{\n  \"N\":"<<N<<",\"Q\":"<<Q<<",\"T\":"<<T<<",\n";
    js<<"  \"labels\":[";
    for(int t=0;t<T;t++){js<<"\""<<(t+1)<<"\"";if(t<T-1)js<<",";}
    js<<"],\n";
    auto arr=[&](auto& name, auto& v){
        js<<"  \""<<name<<"\":[";
        for(int t=0;t<T;t++){js<<std::fixed<<std::setprecision(3)<<v[t];if(t<T-1)js<<",";}
        js<<"],\n";
    };
    arr("bst",bstTimes); arr("eyt",eytTimes); arr("veb",vebTimes);
    js<<"  \"bst_avg\":"<<ba<<",\"eyt_avg\":"<<ea<<",\"veb_avg\":"<<va<<",\n";
    js<<"  \"sp_eyt\":"<<ba/ea<<",\"sp_veb\":"<<ba/va<<"\n}\n";

    std::cout<<"\nCSV y JSON listos.\n";
    return 0;
}
