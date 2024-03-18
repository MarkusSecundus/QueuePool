#include<iostream>
#include<vector>
#include<random>
#include<climits>

#include "tests.h"
#include "../utils/linked_list.h"



using namespace linked_lists;


namespace tests {

    struct LinkedListNode {
        LinkedListNode(int value_) :next(this), last(this), value(value_) {}
        LinkedListNode* next, * last;
        int value;

        using n = LinkedListNode*;

        struct policy {
            n get_next(n a) { return a->next; }
            n get_last(n a) { return a->last; }
            void set_next(n node, n to_set) { node->next = to_set; }
            void set_last(n node, n to_set) { node->last = to_set; }
            bool is_same_node(n a, n b) { return a == b; }
            bool is_null(n a) { return !a; }
        };

        struct policy_reversed {
            n get_next(n a) { return a->last; }
            n get_last(n a) { return a->next; }
            void set_next(n node, n to_set) { node->last = to_set; }
            void set_last(n node, n to_set) { node->next = to_set; }
            bool is_same_node(n a, n b) { return a == b; }
            bool is_null(n a) { return !a; }
        };
    };

    void ll_test() {
        std::cout << "\n----------------------------------------\nLINKED LIST...\n";
        LinkedListNode a(10), b(11), c(12), d(13), e(14), f(15), g(16);
        linked_list_manipulator_t<LinkedListNode*, LinkedListNode::policy> h;
        linked_list_manipulator_t<LinkedListNode*, LinkedListNode::policy_reversed> h2;

        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.init_node(&d);
        h.init_node(&e);
        h.init_node(&f);
        h.init_node(&g);

#define TEST_PRINT(n) std::cout << "it "<<h.validate_list(&n) <<"(" #n "): "; std::cout << "<"<< h.length(&n) <<"> " ; h.for_each(&n, [](LinkedListNode* n) {std::cout << n->value << ", "; }); std::cout<< " | <"<<h2.length(&n) <<"> "; h2.for_each(&n, [](LinkedListNode* n) {std::cout << n->value << ", ";  }); std::cout<<("\n")

        TEST_PRINT(a);
        h.disconnect_node(&a);
        TEST_PRINT(a);
        h.prepend_list(&a, &b);
        TEST_PRINT(a);
        h.prepend_list(&c, &d);
        h.prepend_list(&c, &e);
        TEST_PRINT(c);
        h.prepend_list(&a, &c);
        TEST_PRINT(a);

        h.disconnect_node(&a);
        TEST_PRINT(a);
        TEST_PRINT(b);
        h.disconnect_node(&c);
        TEST_PRINT(c);
        TEST_PRINT(b);
        h.disconnect_node(&b);
        TEST_PRINT(d);
        TEST_PRINT(b);
        h.disconnect_node(&d);
        TEST_PRINT(d);

        std::cout <<("reconnecting!...\n");
        h.prepend_list(&a, &b);
        h.prepend_list(&a, &c);
        h.prepend_list(&d, &e);
        h.prepend_list(&a, &d);
        TEST_PRINT(a);
        TEST_PRINT(b);
        TEST_PRINT(c);
        TEST_PRINT(d);
        TEST_PRINT(e);
        //return;
        std::cout <<("disconnecting!...\n");
        h.for_each(&a, [&](LinkedListNode* n) {h.disconnect_node(n); std::cout <<n->value <<" "; }); std::cout <<("\n");
        TEST_PRINT(a);
        TEST_PRINT(b);
        TEST_PRINT(c);
        TEST_PRINT(d);
        TEST_PRINT(e);

        std::cout <<("swapping preparation...\n");
        h.prepend_list(&a, &b);
        h.prepend_list(&a, &c);
        h.prepend_list(&d, &e);
        TEST_PRINT(a);
        TEST_PRINT(d);
        std::cout <<("swapping!...\n");
        h.swap_nodes(&a, &d);
        TEST_PRINT(a);
        TEST_PRINT(d);
        std::cout <<("more swapping!...\n");
        h.swap_nodes(&a, &f);
        TEST_PRINT(a);
        TEST_PRINT(f);
        h.swap_nodes(&d, &g);
        TEST_PRINT(d);
        TEST_PRINT(g);
        h.swap_nodes(&a, &d);
        TEST_PRINT(a);
        TEST_PRINT(d);


#undef TEST_PRINT
    }

    void ll_node_swap_test() {
        std::cout << "\n----------------------------------------\nLINKED LIST SWAP...\n";
        using n = LinkedListNode*;


#define TEST_PRINT(n) std::cout << "it "<<h.validate_list(&n) <<"(" #n "): "; std::cout << "<"<< h.length(&n) <<"> " ; h.for_each(&n, [](LinkedListNode* n) {std::cout << n->value << ", "; }); std::cout<<("\n")


        LinkedListNode a(10), b(11), c(12), d(13), e(14), f(15), g(16);
        linked_list_manipulator_t<n, LinkedListNode::policy> h;

        std::cout << "singles...\n";
        h.init_node(&a);
        h.init_node(&b);
        TEST_PRINT(a);
        TEST_PRINT(b);
        h.swap_nodes(&a, &b);
        TEST_PRINT(a);
        TEST_PRINT(b);


        std::cout << "\nsingle-pair...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.prepend_list(&a, &b);
        TEST_PRINT(a);
        TEST_PRINT(c);
        h.swap_nodes(&a, &c);
        TEST_PRINT(a);
        TEST_PRINT(c);
        
        std::cout << "\nsingle-pair2...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.prepend_list(&a, &b);
        TEST_PRINT(a);
        TEST_PRINT(c);
        h.swap_nodes(&c, &a);
        TEST_PRINT(a);
        TEST_PRINT(c);

        std::cout << "\nsingle-triplet...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.init_node(&d);
        h.prepend_list(&a, &b);
        h.prepend_list(&a, &c);
        TEST_PRINT(a);
        TEST_PRINT(d);
        h.swap_nodes(&a, &d);
        TEST_PRINT(a);
        TEST_PRINT(d);

        std::cout << "\nsingle-triplet2...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.init_node(&d);
        h.prepend_list(&a, &b);
        h.prepend_list(&a, &c);
        TEST_PRINT(a);
        TEST_PRINT(d);
        h.swap_nodes(&d, &a);
        TEST_PRINT(a);
        TEST_PRINT(d);

        std::cout << "\npair-pair...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.init_node(&d);
        h.prepend_list(&a, &b);
        h.prepend_list(&c, &d);
        TEST_PRINT(a);
        TEST_PRINT(c);
        h.swap_nodes(&a, &c);
        TEST_PRINT(a);
        TEST_PRINT(c);

        std::cout << "\npair-triplet...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.init_node(&d);
        h.init_node(&e);
        h.prepend_list(&a, &b);
        h.prepend_list(&c, &d);
        h.prepend_list(&c, &e);
        TEST_PRINT(a);
        TEST_PRINT(c);
        h.swap_nodes(&a, &c);
        TEST_PRINT(a);
        TEST_PRINT(c);

        std::cout << "\npair-triplet2...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.init_node(&d);
        h.init_node(&e);
        h.prepend_list(&a, &b);
        h.prepend_list(&c, &d);
        h.prepend_list(&c, &e);
        TEST_PRINT(a);
        TEST_PRINT(c);
        h.swap_nodes(&c, &a);
        TEST_PRINT(a);
        TEST_PRINT(c);

        std::cout << "\ntriplet-triplet...\n";
        h.init_node(&a);
        h.init_node(&b);
        h.init_node(&c);
        h.init_node(&d);
        h.init_node(&e);
        h.init_node(&f);
        h.prepend_list(&a, &b);
        h.prepend_list(&a, &c);
        h.prepend_list(&d, &e);
        h.prepend_list(&d, &f);
        TEST_PRINT(a);
        TEST_PRINT(d);
        h.swap_nodes(&d, &a);
        TEST_PRINT(a);
        TEST_PRINT(d);


#undef TEST_PRINT
    }


    template<int LIST_CREATE_PROB, int LIST_DESTROY_PROB, int ELEMENT_REMOVE_PROB, int ELEMENT_SWAP_PROB, std::size_t ITERATIONS>
    void ll_randomized_test_impl() {
        std::cout << "\n----------------------------------------\nLINKED LIST RANDOMIZED(iterations="<<ITERATIONS<<", list_create=1/"<< LIST_CREATE_PROB<<", list_destroy=1/"<<LIST_DESTROY_PROB<<", remove_elem=1/"<<ELEMENT_REMOVE_PROB<<", swap_elem=1/"<<ELEMENT_SWAP_PROB<<")...\n";
        using n = LinkedListNode*;
        using nr = LinkedListNode;
        using v = std::vector<int>;
        linked_list_manipulator_t<n, LinkedListNode::policy> h;


        std::random_device rd;  // a seed source for the random number engine
        std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<int> distrib(0, INT_MAX);

        std::vector<n> lists;
        std::vector<v> std_lists;

        std::size_t max_length = 0, max_lists_count=0;

        auto validate = [&](std::size_t t, int index, n node, v* std) {

            if(!h.validate_list(node))
                std::cout << t << ")... id(" << index << ") - validation error\n";
            auto ll_length = h.length(node);
            if (ll_length != std->size())
                std::cout << t << ")... id(" << index << ") - sizes: " << ll_length << " vs " << std->size() << "\n";

            max_length = std::max(max_length, ll_length);

            std::size_t i = 0;
            h.for_each(node, [&](n nn) {
                if (nn->value != std->operator[](i))
                    std::cout << t << ")... id(" << index << ") - values at pos " << i << ": >" << nn->value << " vs " << std->operator[](i) << "\n";
                ++i;
                });
            };
        
        for (std::size_t t = 0; t < ITERATIONS; ++t) {
            int rand = distrib(gen);
            int index = rand % (std_lists.size()?std_lists.size():1);

            max_lists_count = std::max(max_lists_count, std_lists.size());

            if (std_lists.size() > 0)
                validate(t, index, lists[index], &std_lists[index]);

            if ((std_lists.size()==0) || !(distrib(gen) % LIST_CREATE_PROB)) {
                n new_node = new nr(rand);
                h.init_node(new_node);
                lists.emplace_back(new_node);
                std_lists.emplace_back(v{ rand });
            }
            else if (!(distrib(gen) % LIST_DESTROY_PROB)) {
                int index2 = distrib(gen) % std_lists.size();
                if (index == index2) continue;
                n to_join = lists[index2];
                v to_join_std = std::move(std_lists[index2]);
                lists[index] = h.prepend_list(lists[index], to_join);
                for (int i : to_join_std) std_lists[index].push_back(i);
                lists.erase(lists.begin() + index2);
                std_lists.erase(std_lists.begin() + index2);
            }
            else if (!(distrib(gen) % ELEMENT_SWAP_PROB)) {
                int index2 = distrib(gen) % std_lists.size();
                if (index == index2) continue;
                //TODO: implement
            }
            else if (!(distrib(gen) % ELEMENT_REMOVE_PROB)) {
                continue;
                if (std_lists[index].size() <= 1) {
                    delete lists[index];
                    lists.erase(lists.begin() + index);
                    std_lists.erase(std_lists.begin() + index);
                }
                else {
                    auto to_delete = lists[index];
                    lists[index] = h.disconnect_node(lists[index]);
                    delete to_delete;
                    std_lists[index].erase(std_lists[index].begin());
                }
            }
            else {
                n new_node = new nr(rand);
                h.init_node(new_node);
                h.prepend_list(lists[index], new_node);
                std_lists[index].emplace_back(rand);
            }
        }
        std::cout << "test finished(max_list_length=" << max_length << ", max_lists_count="<<max_lists_count<< ").\n";
    }

    void ll_randomized_test() {
        ll_randomized_test_impl<300,300,6, 100, 50000>();
        ll_randomized_test_impl<100,100,6, 100, 50000>();
        ll_randomized_test_impl<200,200,2, 100, 50000>();
        ll_randomized_test_impl<5,5,2, 100, 50000>();
        ll_randomized_test_impl<2,2,2, 100, 50000>();
    }
}
