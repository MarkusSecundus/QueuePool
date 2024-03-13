#include<iostream>

#include "tests.h"
#include "../linked_list.h"

using namespace linked_lists;


namespace tests {

    struct LinkedListNode {
        LinkedListNode* next, * last;
        int value;

        using n = LinkedListNode*;

        struct policy {
            n get_next(n a) { return a->next; }
            n get_last(n a) { return a->last; }
            void set_next(n node, n to_set) { node->next = to_set; }
            void set_last(n node, n to_set) { node->last = to_set; }
            bool is_same_node(n a, n b) { return a == b; }
        };

        struct policy_reversed {
            n get_next(n a) { return a->last; }
            n get_last(n a) { return a->next; }
            void set_next(n node, n to_set) { node->last = to_set; }
            void set_last(n node, n to_set) { node->next = to_set; }
            bool is_same_node(n a, n b) { return a == b; }
        };
    };

    void ll_test() {
        std::cout << "\n----------------------------------------\nLINKED LIST...\n";
        LinkedListNode a, b, c, d, e, f, g;
        linked_list_manipulator_t<LinkedListNode*, LinkedListNode::policy> h;
        linked_list_manipulator_t<LinkedListNode*, LinkedListNode::policy_reversed> h2;

        a.value = 10; h.init_node(&a);
        b.value = 11; h.init_node(&b);
        c.value = 12; h.init_node(&c);
        d.value = 13; h.init_node(&d);
        e.value = 14; h.init_node(&e);
        f.value = 15; h.init_node(&f);
        g.value = 16; h.init_node(&g);

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
}
