#include<iostream>

#include "tests.h"
#include "../linked_list.h"

using namespace linked_lists;


namespace tests {

    struct LinkedListNode {
        LinkedListNode(int value_) :value(value_), last(this), next(this) {}
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


    }
#undef TEST_PRINT
}
