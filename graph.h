#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#include <cstdint>
#include <iostream>
#include <stack>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>

#ifdef NO_PLOT
#else
#include <graphviz/cgraph.h>
#endif

using node = uint32_t;

class CNet {
    private:
    std::vector<std::vector<node>> Nodes;
    
    public:
    CNet() {}
    CNet(const std::vector<std::vector<node>>& net) : Nodes(net) {}
    CNet(const size_t N) {
        Nodes.resize(N);
    }
    std::vector<std::vector<node>> getNodes() const {
        return Nodes;
    }
    // Reserve space for N nodes
    void reserve(const size_t N) {
        Nodes.reserve(N);
    }
    size_t size(){
        return Nodes.size();
    }
    // Adds a new node at the end with no neighbors
    void add() {
        Nodes.push_back({});
    }
    // Adds a new node at the end with the given neighbors
    void add(const std::vector<node>& neighbors) {
        Nodes.push_back(neighbors);
    }
    // Assign the neighbors of node i to be an empty vector
    void assign(const node i) {
        if (i >= Nodes.size()) return;
        Nodes[i] = {};
    }
    // Assign the neighbors of node i to be the given vector
    void assign(const node i, const std::vector<node>& neighbors) {
        if (i >= Nodes.size()) return;
        Nodes[i] = neighbors;
    }
    
    CNet operator+(const CNet& net) {
        std::vector<std::vector<node>> joined;
        joined.reserve(Nodes.size() + net.getNodes().size());

        // Copy the nodes from the current network
        for (const auto& nodeVec : Nodes) {
            joined.push_back(nodeVec);
        }

        // Offset the nodes from the input network and append to joined
        for (const auto& nodeVec : net.getNodes()) {
            std::vector<node> offsetNodeVec;
            offsetNodeVec.reserve(nodeVec.size());

            // Offset each node index
            for (const auto& n : nodeVec) {
                offsetNodeVec.push_back(n + Nodes.size());
            }

            joined.push_back(std::move(offsetNodeVec));
        }

        return CNet(joined);
    }
    CNet& operator+=(const CNet& net) {
        node offset = Nodes.size();
        Nodes.reserve(Nodes.size() + net.getNodes().size());
        // Offset the nodes from the input network and append to joined
        for (const auto& nodeVec : net.getNodes()) {
            std::vector<node> offsetNodes;
            offsetNodes.reserve(nodeVec.size());

            // Offset each node index
            for (const auto& n : nodeVec) {
                offsetNodes.push_back(n + offset);
            }

            Nodes.push_back(std::move(offsetNodes));
        }
        return *this;
    }
    
    // Unlinks two nodes
    void inline unlink(const node i, const node j) {
        Nodes[i].erase(std::find(Nodes[i].begin(), Nodes[i].end(), j));
        Nodes[j].erase(std::find(Nodes[j].begin(), Nodes[j].end(), i));
    }
    // Links two nodes if they are not already linked
    void inline link(node i, node j) {
        if (i == j) return;

        if (Nodes[i].size()>Nodes[j].size()) std::swap(i, j);

        if (std::find(Nodes[i].begin(), Nodes[i].end(), j) == Nodes[i].end()) {
            Nodes[i].push_back(j);
            Nodes[j].push_back(i);
        }
    }
    // (WARNING) This function is used to link two nodes without checking if they are already linked (use link insrtad)
    void inline link_no_check(const node i, const node j) {
        if (i == j) return;
        Nodes[i].push_back(j);
        Nodes[j].push_back(i);
    }
    // Prints the network
    void print() {
        for (size_t i = 0; i < Nodes.size(); i++) {
            std::cout << i << ": ";
            for (const node& j : Nodes[i]) {
                std::cout << j << " ";
            }
            std::cout << std::endl;
        }
    }
    // Returns a connected subgraph of the network containing the nodes in the input vector
    // (WARNING) All nodes must be connected, and the input vector must contain all nodes in the subgraph
    CNet connected_subgraph(const std::vector<node>& nodes) {
        std::vector<std::vector<node>> subgraph(nodes.size());
        std::unordered_map<node, node> node_to_index;

        for (const node& i : nodes) {
            node_to_index[i] = node_to_index.size();
        }
        for (const node& i : nodes) {
            for (const node& j : Nodes[i]) {
                subgraph[node_to_index[i]].push_back(node_to_index[j]);
            }
        }
        return CNet(subgraph);
    }
    // Returns the giant component of the network
    CNet get_giant_component() {
        if (Nodes.empty()) {
            return CNet();
        }

        std::vector<bool> visited(Nodes.size(), false);
        std::vector<node> best, recorded;

        node count = 0;

        node current;
        std::stack<node> dfs_stack;
        for (node i = 0; i < Nodes.size(); ++i) {
            if (visited[i]) continue;
            recorded.clear();
            dfs_stack.push(i);

            while (!dfs_stack.empty()) {
                current = dfs_stack.top();
                dfs_stack.pop();

                if (!visited[current]) {
                    visited[current] = true;
                    recorded.push_back(current);

                    for (const node& neighbor : Nodes[current]) {
                        if (!visited[neighbor]) {
                            dfs_stack.push(neighbor);
                        }
                    }
                }
            }
            count += recorded.size();
            if (recorded.size() > best.size()) best.swap(recorded);//best = recorded;
            if (best.size() >= (Nodes.size()-count)) break;
        }

        return this->connected_subgraph(best);
    }
    // Returns the degree distribution of the network
    std::vector<size_t> degree_distribution() {
        std::vector<size_t> degree_dist;
        for (const auto& neighbors : Nodes){
            if (degree_dist.size() <= neighbors.size()) {
                degree_dist.resize(neighbors.size()+1);
            }
            degree_dist[neighbors.size()]++;
        }
        return degree_dist;
    }
    // Prints the degree distribution of the network
    void print_degree_distribution(const int width=2) {
        auto vec = degree_distribution();
        std::cout << "Degree distribution:\t";
        for (auto i : vec) {
            std::cout << std::setw(width) << i << " ";
        }
        std::cout << std::endl;
    }
    // Plots the degree distribution of the network using Gnuplot
    void plot_degree_distribution(const std::string& term="qt", const bool pause=true) {
        std::vector<size_t> data = degree_distribution();
        // Create a pipe to Gnuplot
        FILE *gnuplotPipe = popen("gnuplot -persist", "w");
        if (!gnuplotPipe) {
            std::cerr << "Error opening Gnuplot pipe!" << std::endl;
            return;
        }
        //fprintf(gnuplotPipe, "show term\n");
        // Send commands to Gnuplot to plot the histogram
        //fprintf(gnuplotPipe, "set term wxt enhanced\n");
        fprintf(gnuplotPipe, "set term %s\n", term.c_str());
        fprintf(gnuplotPipe, "set boxwidth 0.5\n");
        fprintf(gnuplotPipe, "set style fill solid\n");
        fprintf(gnuplotPipe, "plot '-' using 1:2 with boxes notitle\n");

        
        size_t start = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] != 0){
                start = i;
                break;
            }
        }
        // Send data to Gnuplot
        for (size_t i = start; i < data.size(); ++i) {
            fprintf(gnuplotPipe, "%zu %zu\n", i, data[i]);
        }
        fprintf(gnuplotPipe, "e\n"); // End of data
        
        if (pause) fprintf(gnuplotPipe, "pause mouse close\n");
        // Close the Gnuplot pipe
        pclose(gnuplotPipe);
    }
    // Creates a ring network, where each node has grade neighbors
    static CNet ring(const node N, const size_t grade=2) {
        if (grade < 2 || grade >= N) {
            throw std::domain_error("Grade must be between 2 and N-1");
        }
        CNet ring(N);
        for (node i = 0; i < N; i++) {
            for (size_t j = 1; j <= grade/2; j++) {
                ring.link_no_check(i, (i+j)%N);
            }
        }
        return ring;
    }
    // Creates a Watts-Strogatz network
    static CNet WattsStrogatz(const node N, const size_t grade, const double beta) {
        CNet ring = CNet::ring(N, grade);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0, 1);
        std::uniform_int_distribution<> dis(0, N - 3);
        
        size_t num_rand;
        for (node i = 0; i < N; i++) {
            for (size_t j = 1; j <= grade/2; j++) {
                if (dist(gen) < beta) {
                    ring.unlink(i, (i+j)%N);
                    num_rand = dis(gen);
                    if (num_rand >= i) num_rand++;
                    if (num_rand >= j) num_rand++;
                    while (std::find(ring.Nodes[i].begin(), ring.Nodes[i].end(), num_rand) != ring.Nodes[i].end()) {
                        num_rand = dis(gen);
                        if (num_rand >= i) num_rand++;
                        if (num_rand >= j) num_rand++;
                    }
                    ring.link_no_check(i, num_rand);
                }
            }
        }
        return ring;
    }
    static CNet WattsStrogatz_rewire(const node N, const size_t grade, const double beta) {
        CNet ring = CNet::ring(N, grade);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0, 1);
        std::uniform_int_distribution<> dis(0, N - 3);
        
        node num_rand, rewire, left;
        size_t warning_count;
        for (node i = 0; i < N; i++) {
            for (node j = 1; j <= grade/2; j++) {
                if (dist(gen) < beta) {
                    left = (i+j)%N;
                    ring.unlink(i, left);
                    num_rand = dis(gen);
                    if (num_rand >= i) num_rand++;
                    if (num_rand >= j) num_rand++;
                    while (std::find(ring.Nodes[i].begin(), ring.Nodes[i].end(), num_rand) != ring.Nodes[i].end()) {
                        num_rand = dis(gen);
                        if (num_rand >= i) num_rand++;
                        if (num_rand >= j) num_rand++;
                    }
                    ring.link_no_check(i, num_rand);
                    rewire = dist(gen)*ring.Nodes[num_rand].size();
                    warning_count=0;
                    while (std::find(ring.Nodes[rewire].begin(), ring.Nodes[rewire].end(), left) != ring.Nodes[rewire].end()) {
                        rewire = dist(gen)*ring.Nodes[num_rand].size();
                        warning_count++;
                        if (warning_count > ring.Nodes[rewire].size()*10){
                            std::cout << "Warning, infinite loop, exiting\n";
                            break;
                        }
                    }
                    ring.unlink(num_rand, rewire);
                    ring.link_no_check(left, rewire);
                    
                }
            }
        }
        return ring;
    }
    // Creates a Barabasi-Albert network
    static CNet BarabasiAlbert(const node N, const size_t m) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, N);
        CNet net = CNet::ring(m+1, m);
        std::vector<size_t> degree_dist = net.degree_distribution();
        std::vector<double> prob(N);
        std::vector<node> nodes;
        for (node i = 0; i < N; i++) {
            nodes.push_back(i);
        }
        for (size_t i = m+1; i < N; i++) {
            for (size_t j = 0; j < m; j++) {
                prob[j] = degree_dist[j]/(2*(i-1));
            }
            std::discrete_distribution<> d(prob.begin(), prob.end());
            for (size_t j = 0; j < m; j++) {
                net.link_no_check(i, d(gen));
            }
            degree_dist.push_back(m);
        }
        return net;
    }
    // Creates an Erdos-Renyi network
    static CNet ErdosRenyi(const node N, const double p) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0, 1);
        CNet net(N);
        for (node i = 0; i < N; i++) {
            for (node j = i+1; j < N; j++) {
                if (dist(gen) < p) {
                    net.link_no_check(i, j);
                }
            }
        }
        return net;
    }

    void plot() {
        // Initialize Graphviz
        Agraph_t *g = agopen("undirected_graph", Agdirected, 0);

        // Create vertices
        std::vector<Agnode_t*> nodes;
        for (size_t i = 0; i < Nodes.size(); ++i) {
            Agnode_t *node = agnode(g, const_cast<char *>(std::string(std::to_string(i)).c_str()), 1);
            nodes.push_back(node);
        }

        // Create edges
        for (size_t i = 0; i < Nodes.size(); ++i) {
            for (size_t j = 0; j < Nodes[i].size(); ++j) {
            const node& n1 = i;
            const node& n2 = Nodes[i][j];
            Agedge_t *e = agedge(g, nodes[i], nodes[j], NULL, 1);
            //agset(e, "label", "edge", ""); // Set edge label (optional)
            }
        }

        // Generate the DOT file
        std::string dotFilename = "graph.dot";
        FILE *dotFile = fopen(dotFilename.c_str(), "w");
        agwrite(g, dotFile);
        fclose(dotFile);

        // Render the graph using Graphviz's layout engine
        std::string command = "dot -Tpng " + dotFilename + " -o graph.png";
        system(command.c_str());
    }

};
class Net {
    private:
    std::unordered_map<node, std::unordered_set<node>> Nodes;
    static Net _WattsStrogatz(const node N, const size_t grade, const double beta) {
        Net ring = Net::ring(N, grade);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0, 1);
        std::uniform_int_distribution<> dis(0, N - 3);
        
        size_t num_rand;
        for (node i = 0; i < N; i++) {
            for (size_t j = 1; j <= grade/2; j++) {
                if (dist(gen) < beta) {
                    ring.unlink(i, (i+j)%N);
                    num_rand = dis(gen);
                    if (num_rand >= i) num_rand++;
                    if (num_rand >= j) num_rand++;
                    while (ring.getNodes()[i].count(num_rand)) {
                        num_rand = dis(gen);
                        if (num_rand >= i) num_rand++;
                        if (num_rand >= j) num_rand++;
                    }
                    ring.link(i, num_rand);
                }
            }
        }
        return ring;
    }

    public:
    Net() {}
    Net(std::unordered_map<node, std::unordered_set<node>>& neighbors) : Nodes(neighbors) {}
    Net(const std::vector<std::unordered_set<node>>& neighbors) {
        for (size_t i = 0; i < neighbors.size(); i++) {
            Nodes[i] = neighbors[i];
        }
    }
    Net(const size_t N) {
        for (node i = 0; i < N; i++) {
            Nodes[i] = std::unordered_set<node>();
        }
    }
    /* Net(const CNet& cnet) {
        for (size_t i = 0; i < cnet.getNodes().size(); i++) {
            std::unordered_set<node> nodeSet;
            std::vector<std::vector<node>> cnetnodes = cnet.getNodes();
            for (const node& n : cnetnodes[i]) {
                nodeSet.insert(n);
            }
            Nodes[i] = nodeSet;
    } */
    Net(const CNet& cnet) {
        const auto& cnetNodes = cnet.getNodes();
        Nodes.reserve(cnetNodes.size()); // Reserve space for efficiency

        for (size_t i = 0; i < cnetNodes.size(); ++i) {
            const auto& cnetNodeSet = cnetNodes[i];
            std::unordered_set<node> nodeSet;
            nodeSet.reserve(cnetNodeSet.size()); // Reserve space for efficiency

            for (const node& n : cnetNodeSet) {
                nodeSet.insert(n);
            }

            Nodes.emplace(i, std::move(nodeSet)); // Use move semantics for efficiency
        }
    }


    // Adds a new node at i with no neighbors
    void add(const node i) {
        Nodes[i] = std::unordered_set<node>();
    }
    // Adds a new node at i with the given neighbors
    void add(const node i, const std::unordered_set<node>& neighbors) {
        Nodes[i] = neighbors;
    }
    // Adds a new node at available index with no neighbors
    node add() {
        for (node i = 0; i < Nodes.size(); i++) {
            if (Nodes.find(i) == Nodes.end()) {
                Nodes[i] = std::unordered_set<node>();
                return i;
            }
        }
    }
    // Adds a new node at available index with the given neighbors
    node add(const std::unordered_set<node>& neighbors) {
        for (node i = 0; i < Nodes.size(); i++) {
            if (Nodes.find(i) == Nodes.end()) {
                Nodes[i] = neighbors;
                return i;
            }
        }
    }
    // Removes the node i
    void remove(const node i) {
        Nodes.erase(i);
        for (auto& [_, neighbors] : Nodes) {
            neighbors.erase(i);
        }
    }
    // clears the net
    void clear() {
        Nodes.clear();
    }

    // links node i with node j
    void link(const node i, const node j) {
        if (i == j) return;
        Nodes[i].insert(j);
        Nodes[j].insert(i);
    }
    // links node i with all nodes in the set neighbors
    void link(const node i, const std::unordered_set<node>& neighbors) {
        for (const node& j : neighbors) {
            Nodes[i].insert(j);
            Nodes[j].insert(i);
        }
    }
    // unlinks node i with node j
    void unlink(const node i, const node j) {
        Nodes[i].erase(j);
        Nodes[j].erase(i);
    }
    // unlinks node i with all nodes in the set neighbors
    void unlink(const node i, const std::unordered_set<node>& neighbors) {
        for (const node& j : neighbors) {
            Nodes[i].erase(j);
            Nodes[j].erase(i);
        }
    }

    std::unordered_map<node, std::unordered_set<node>> getNodes() const {
        return Nodes;
    }

    std::vector<node> index_to_node() {
        std::vector<node> index_to_node(Nodes.size());
        size_t j = 0;
        for (const auto& [i, _] : Nodes) {
            index_to_node[j]= i;
            ++j;
        }
        return index_to_node;
    }
    std::unordered_map<node, node> node_to_index(std::vector<node> index_to_node) {
        std::unordered_map<node, node> node_to_index;
        for (size_t i = 0; i < index_to_node.size(); ++i) {
            node_to_index[index_to_node[i]] = i;
        }
        return node_to_index;
    }

    Net get_giant_component() {
        if (Nodes.empty()) {
            return Net();
        }

        std::vector<bool> visited(Nodes.size(), false);
        std::vector<node> best, recorded;
        std::vector<node> index_to_node = this->index_to_node();
        std::unordered_map<node, node> node_to_index = this->node_to_index(index_to_node);

        size_t count = 0;

        node start, current, current_index;
        std::stack<node> dfs_stack;
        for (node i = 0; i < Nodes.size(); ++i) {
            start = index_to_node[i];
            if (visited[i]) continue;
            recorded.clear();
            dfs_stack.push(start);

            while (!dfs_stack.empty()) {
                current = dfs_stack.top();
                dfs_stack.pop();

                current_index = node_to_index[current];
                if (!visited[current_index]) {
                    visited[current_index] = true;
                    recorded.push_back(current);

                    for (const node& neighbor : Nodes[current]) {
                        if (!visited[node_to_index[neighbor]]) {
                            dfs_stack.push(neighbor);
                        }
                    }
                }
            }
            count += recorded.size();
            if (recorded.size() > best.size()) best = recorded;
            if (best.size() >= (Nodes.size()-count)) break;
        }

        std::unordered_map<node, std::unordered_set<node>> giant_component;
        for (const node& i : best) {
            giant_component[i] = Nodes[i];
        }

        return Net(giant_component);
    }


    node max(){
        node max = 0;
        for (const auto& [i, _] : Nodes) {
            if (i > max) {
                max = i;
            }
        }
        return max;
    }

    Net join(const Net& net) {
        node sum = this->max() + 1;
        std::unordered_map<node, std::unordered_set<node>> joined = Nodes;
        std::unordered_set<node> neighbors_sum;
        for (const auto& [i, neighbors] : net.getNodes()) {
            neighbors_sum.clear();
            for (const node& j : neighbors) {
                neighbors_sum.insert(j+sum);
            }
            joined[i+sum] = neighbors_sum;
        }
        return Net(joined);
    }
    std::vector<size_t> degree_list() {
        std::vector<size_t> degree_dist;
        for (const auto& [_, neighbors] : Nodes) {
            degree_dist.push_back(neighbors.size());
        }
        return degree_dist;
    }
    std::vector<size_t> degree_distribution() {
        std::vector<size_t> degree_dist;
        for (const auto& [_, neighbors] : Nodes) {
            if (degree_dist.size() <= neighbors.size()) {
                degree_dist.resize(neighbors.size()+1);
            }
            degree_dist[neighbors.size()]++;
        }
        return degree_dist;
    }
    void plot_degree_distribution() {
        std::vector<size_t> data = degree_distribution();
        // Create a pipe to Gnuplot
        FILE *gnuplotPipe = popen("gnuplot -persist", "w");
        if (!gnuplotPipe) {
            std::cerr << "Error opening Gnuplot pipe!" << std::endl;
            return;
        }
        //fprintf(gnuplotPipe, "show term\n");
        // Send commands to Gnuplot to plot the histogram
        fprintf(gnuplotPipe, "set boxwidth 0.5\n");
        fprintf(gnuplotPipe, "set style fill solid\n");
        fprintf(gnuplotPipe, "plot '-' using 1:2 with boxes notitle\n");

        // Send data to Gnuplot
        for (size_t i = 0; i < data.size(); ++i) {
            fprintf(gnuplotPipe, "%zu %zu\n", i, data[i]);
        }
        fprintf(gnuplotPipe, "e\n"); // End of data

        // Close the Gnuplot pipe
        pclose(gnuplotPipe);
    }

    void print() {
        std::cout << *this;
    }
    static Net ring(const node N, const size_t grade=1) {
        std::unordered_map<node, std::unordered_set<node>> ring;
        for (node i = 0; i < N; i++) {
            ring[i] = {};
        }
        Net net(ring);
        for (node i = 0; i < N; i++) {
            for (size_t j = 1; j <= grade/2; j++) {
                net.link(i, (i+j)%N);
            }
        }
        return net;
    }
    
    // binder to optimized WattsStrogatz from CNet
    static Net WattsStrogatz(const node N, const size_t grade, const double beta) {
        return Net(CNet::WattsStrogatz(N, grade, beta));
    }

    static Net BarabasiAlbert(const node N, const size_t m) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, N);
        Net net = Net::ring(m+1, m);
        std::vector<node> index_to_node = net.index_to_node();
        std::unordered_map<node, node> node_to_index = net.node_to_index(index_to_node);
        std::vector<size_t> degree_dist = net.degree_list();
        std::vector<double> prob(N);
        std::vector<node> nodes;
        for (node i = 0; i < N; i++) {
            nodes.push_back(i);
        }
        for (size_t i = m+1; i < N; i++) {
            for (size_t j = 0; j < m; j++) {
                prob[j] = degree_dist[j]/(2*(i-1));
            }
            std::discrete_distribution<> d(prob.begin(), prob.end());
            for (size_t j = 0; j < m; j++) {
                net.link(i, index_to_node[d(gen)]);
            }
            degree_dist.push_back(m);
        }
        return net;
    }
    static Net ErdosRenyi(const node N, const double p) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0, 1);
        Net net;
        for (node i = 0; i < N; i++) {
            net.add(i);
        }
        for (node i = 0; i < N; i++) {
            for (node j = i+1; j < N; j++) {
                if (dist(gen) < p) {
                    net.link(i, j);
                }
            }
        }
        return net;
    }
    // overload the operator+ to join two nets
    Net operator+(const Net& net) {
        return this->join(net);
    }
    // overload the operator+= to join two nets
    Net& operator+=(const Net& net) {
        node sum = this->max() + 1;
        std::unordered_set<node> neighbors_sum;
        for (const auto& [i, neighbors] : net.getNodes()) {
            neighbors_sum.clear();
            for (const node& j : neighbors) {
                neighbors_sum.insert(j+sum);
            }
            Nodes[i+sum] = neighbors_sum;
        }
        return *this;
    }
    // overload the << operator to print the net
    friend std::ostream& operator<<(std::ostream& os, const Net& net) {
        if (net.Nodes.empty()) {
            os << "Empty net" << std::endl;
            return os;
        }
        for (const auto& [i, neighbors] : net.Nodes) {
            os << i << ": ";
            for (const node& j : neighbors) {
                os << j << " ";
            }
            os << std::endl;
        }
        return os;
    }
};


class MNet{
    private:
    std::vector<std::unordered_set<node>> Nodes;
    public:
    MNet() {}
    MNet(const std::vector<std::unordered_set<node>>& neighbors) : Nodes(neighbors) {}
    MNet(const size_t N) {
        Nodes.resize(N);
    }
    MNet operator+(const MNet& net) {
        std::vector<std::unordered_set<node>> joined = Nodes;
        for (size_t i = 0; i < net.getNodes().size(); i++) {
            joined.push_back(net.getNodes()[i]);
        }
        return MNet(joined);
    }
    std::vector<std::unordered_set<node>> getNodes() const {
        return Nodes;
    }
    void link(const node i, const node j) {
        if (i == j) return;
        Nodes[i].insert(j);
        Nodes[j].insert(i);
    }
    void unlink(const node i, const node j) {
        Nodes[i].erase(j);
        Nodes[j].erase(i);
    }
    MNet get_giant_component() {
        if (Nodes.empty()) {
            return MNet();
        }

        std::vector<bool> visited(Nodes.size(), false);
        std::vector<node> best, recorded;

        node count = 0;

        node current;
        std::stack<node> dfs_stack;
        for (node i = 0; i < Nodes.size(); ++i) {
            if (visited[i]) continue;
            recorded.clear();
            dfs_stack.push(i);

            while (!dfs_stack.empty()) {
                current = dfs_stack.top();
                dfs_stack.pop();

                if (!visited[current]) {
                    visited[current] = true;
                    recorded.push_back(current);

                    for (const node& neighbor : Nodes[current]) {
                        if (!visited[neighbor]) {
                            dfs_stack.push(neighbor);
                        }
                    }
                }
            }
            count += recorded.size();
            if (recorded.size() > best.size()) best = recorded;
            if (best.size() >= (Nodes.size()-count)) break;
        }

        std::vector<std::unordered_set<node>> giant_component;
        for (const node& i : best) {
            giant_component.push_back(Nodes[i]);
        }

        return MNet(giant_component);
    }
    void print() {
        for (size_t i = 0; i < Nodes.size(); i++) {
            std::cout << i << ": ";
            for (const node& j : Nodes[i]) {
                std::cout << j << " ";
            }
            std::cout << std::endl;
        }
    }
    std::vector<size_t> degree_distribution() {
        std::vector<size_t> degree_dist;
        for (const auto& neighbors : Nodes){
            if (degree_dist.size() <= neighbors.size()) {
                degree_dist.resize(neighbors.size()+1);
            }
            degree_dist[neighbors.size()]++;
        }
        return degree_dist;
    }
    void plot_degree_distribution() {
        std::vector<size_t> data = degree_distribution();
        // Create a pipe to Gnuplot
        FILE *gnuplotPipe = popen("gnuplot -persist", "w");
        if (!gnuplotPipe) {
            std::cerr << "Error opening Gnuplot pipe!" << std::endl;
            return;
        }
        //fprintf(gnuplotPipe, "show term\n");
        // Send commands to Gnuplot to plot the histogram
        fprintf(gnuplotPipe, "set boxwidth 0.5\n");
        fprintf(gnuplotPipe, "set style fill solid\n");
        fprintf(gnuplotPipe, "plot '-' using 1:2 with boxes notitle\n");

        // Send data to Gnuplot
        for (size_t i = 0; i < data.size(); ++i) {
            fprintf(gnuplotPipe, "%zu %zu\n", i, data[i]);
        }
        fprintf(gnuplotPipe, "e\n"); // End of data

        // Close the Gnuplot pipe
        pclose(gnuplotPipe);
    }
    static MNet ring(const node N, const size_t grade=1) {
        MNet ring(N);
        for (node i = 0; i < N; i++) {
            for (size_t j = 1; j <= grade/2; j++) {
                ring.link(i, (i+j)%N);
            }
        }
        return ring;
    }
    static MNet WattsStrogatz(const node N, const size_t grade, const double beta) {
        MNet ring = MNet::ring(N, grade);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0, 1);
        std::uniform_int_distribution<> dis(0, N - 3);
        
        size_t num_rand;
        for (node i = 0; i < N; i++) {
            for (size_t j = 1; j <= grade/2; j++) {
                if (dist(gen) < beta) {
                    ring.unlink(i, (i+j)%N);
                    num_rand = dis(gen);
                    if (num_rand >= i) num_rand++;
                    if (num_rand >= j) num_rand++;
                    while (ring.Nodes[i].count(num_rand)) {
                        num_rand = dis(gen);
                        if (num_rand >= i) num_rand++;
                        if (num_rand >= j) num_rand++;
                    }
                    ring.link(i, num_rand);
                }
            }
        }
        return ring;
    }
};