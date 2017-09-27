#ifndef __AVL_HPP__
#define __AVL_HPP__

#include <cstdint>
#include <cassert>
#include <cstddef>

#ifdef __AVL_DEBUG__
#include <iostream>
#include <sstream>
#include <fstream>
#endif

namespace avl {

    // direction constants used by nearest()
    // (it is important that the values match child::LEFT and child::RIGHT)
    enum direction {
        BEFORE = 0,
        AFTER = 1
    };

namespace detail {

enum child {
    NONE = -1,
    LEFT = 0,
    RIGHT = 1
};

static constexpr const int child2balance[2] = {-1, 1};
static constexpr const int balance2child[] = {0, 0, 1};

template <typename Tp>
struct node {

    node*   m_children[2];  /* left/right children */
    node*   m_parent;       /* this node's parent */
    child   m_which_child;  /* this node's index in parent's m_children */
    int16_t m_balance;      /* balance: -1, 0, +1 */
    Tp      m_data;

    node(node* parent, avl::detail::child which_child, const Tp& data)
        : m_children(),
            m_parent(parent),
            m_which_child(which_child),
            m_balance(0),
            m_data(data) {
    }

    node(const node& other)
        : m_children(),
          m_parent(other.m_parent),
          m_which_child(other.m_which_child),
          m_balance(other.m_balance),
          m_data(other.m_data) {

        m_children[0] = other.m_children[0];
        m_children[1] = other.m_children[1];
    }

    node& operator=(const node& other) {

        if(this == &other) {
            return *this;
        }

        m_children[0] = other.m_children[0];
        m_children[1] = other.m_children[1];
        m_parent = other.m_parent;
        m_which_child = other.m_which_child;
        m_balance = other.m_balance;
        m_data = other.m_data;

        return *this;
    }

#ifdef __AVL_DEBUG__
    template <typename T2>
    friend std::ostream& operator<<(std::ostream& os, const node<T2>& n);
#endif
};

#ifdef __AVL_DEBUG__
template <class Tp>
std::ostream& operator<<(std::ostream& os, const node<Tp>& n) {

    os << "node<Tp> {\n"
       << "  m_children[0] = " << n.m_children[0] << ";\n"
       << "  m_children[1] = " << n.m_children[1] << ";\n"
       << "  m_parent = " << n.m_parent << ";\n"
       << "  m_which_child = " << n.m_which_child << ";\n"
       << "  m_balance = " << n.m_balance << ";\n"
       << "};";

    return os;
}
#endif // __AVL_DEBUG__

template <typename Tp>
struct default_delete {

    constexpr default_delete() noexcept = default;

    void operator()(Tp ptr) const {
        delete ptr;
    }
};

} // namespace detail




template <typename Tp, typename Dp = detail::default_delete<detail::node<Tp>*> >
class avltree {

    static_assert(std::is_standard_layout<Tp>::value,
            "avltree<Tp> requires Tp to be a standard layout type");

    static_assert(std::is_standard_layout<detail::node<Tp>>::value,
            "avl::node MUST BE a standard layout type!");

public:
    class insertion_point {

        friend class avltree;

public:

        static const insertion_point null;

        insertion_point() 
            : m_parent(nullptr),
              m_child(detail::child::NONE) { }

        insertion_point(detail::node<Tp>* parent, detail::child which_child) 
            : m_parent(parent),
              m_child(which_child) { }

        bool operator==(const insertion_point& other) const {
            return (m_parent == other.m_parent && 
                    m_child == other.m_child);
        }

private:
        detail::node<Tp>*   m_parent;
        detail::child      m_child;

    };

    using index = insertion_point;

    typedef Tp element_type;
    typedef Dp deleter_type;

    // Constructors
    avltree()
    : m_deleter(deleter_type()) {
        static_assert(!std::is_pointer<deleter_type>::value,
		     "constructed with null function pointer deleter");
	}

    avltree(typename std::conditional<std::is_reference<deleter_type>::value,
            deleter_type, const deleter_type&>::type deleter) 
    : m_deleter(deleter) { }

    ~avltree() {
        index tree_idx = index::null;
        detail::node<Tp>* node;

        while((node = destroy_nodes(tree_idx)) != nullptr) {
            //m_deleter(node);
            delete node;
        }
    }

    size_t size() const {
        return m_num_nodes;
    }

    // add a new element to the AVL tree
    // returns a pointer to the newly added element
    Tp* add(const Tp& new_data) {
        //detail::node<Tp>* parent = nullptr;
        //detail::child which_child;

        insertion_point where;
        Tp* res = find(new_data, where);

        // element must not exist (duplicates not allowed)
        assert(res == nullptr);

        return insert_at(new_data, where);
    }

    // delete a node from the AVL tree
    void remove(const Tp& data) {

        insertion_point where = insertion_point::null;

        detail::node<Tp>* res = find_node(data, where);

        // requested data does not exist, we're done
        if(res == nullptr){
            return;
        }

        remove_at(where);
    }

    void remove_node(detail::node<Tp>* to_rm) {

        if(to_rm == nullptr) {
            return;
        }

        detail::node<Tp>* old_node = to_rm;

        /*
         * Deletion is easiest with a node that has at most 1 child.
         * We swap a node with 2 children with a sequentially valued
         * neighbor node. That node will have at most 1 child. Note this
         * has no effect on the ordering of the remaining nodes.
         *
         * As an optimization, we choose the greater neighbor if the tree
         * is right heavy, otherwise the left neighbor. This reduces the
         * number of rotations needed.
         */
        if(to_rm->m_children[detail::child::LEFT] != nullptr &&
           to_rm->m_children[detail::child::RIGHT] != nullptr) {

            /* choose node to swap from whichever side is taller */
            int old_balance = to_rm->m_balance;
            int left = detail::balance2child[old_balance + 1];
            int right = 1 - left;

            detail::node<Tp>* node;

            /* get to the previous value'd node
             * (down 1 left, as far as possible right */
            for(node = to_rm->m_children[left];
                node->m_children[right] != nullptr;
                node = node->m_children[right])
                    ;

            /*
             * create a temp placeholder for 'node'
             * move 'node' to to_rm's spot in the tree
             */
            detail::node<Tp> tmp = *node;

            // (*node = *to_rm)
            // copy everything but Tp
            node->m_children[0] = to_rm->m_children[0];
            node->m_children[1] = to_rm->m_children[1];
            node->m_parent = to_rm->m_parent;
            node->m_which_child = to_rm->m_which_child;
            node->m_balance = to_rm->m_balance;

            if(node->m_children[left] == node) {
                // doesn't matter because we will end up deleting *node*
                node->m_children[left] = &tmp;
            }

            detail::node<Tp>* parent = node->m_parent;

            if(parent != nullptr) {
                parent->m_children[node->m_which_child] = node;
            }
            else {
                m_root = node;
            }

            node->m_children[left]->m_parent = node;
            node->m_children[right]->m_parent = node;

            /*
             * Put tmp where node used to be (just temporary).
             * It always has a parent and at most 1 child.
             */
            to_rm = &tmp;
            parent = to_rm->m_parent;
            parent->m_children[to_rm->m_which_child] = to_rm;
            int which_child = (to_rm->m_children[1] != nullptr);

            if(to_rm->m_children[which_child] != nullptr) {
                to_rm->m_children[which_child]->m_parent = to_rm;
            }
        }

        /*
         * Here we know *to_rm* is at least partially a leaf node. It can
         * be easily removed from the tree.
         */
        assert(m_num_nodes > 0);
        --m_num_nodes;

        detail::node<Tp>* node;
        detail::node<Tp>* parent = to_rm->m_parent;
        int which_child = to_rm->m_which_child;

        if(to_rm->m_children[0] != nullptr) {
            node = to_rm->m_children[0];
        }
        else {
            node = to_rm->m_children[1];
        }

//        std::cout << " *** deleting node: " << old_node << " *** \n";
        m_deleter(old_node);

        /*
         * Connect parent directly to node (leaving out *to_rm*).
         */
        if(node != nullptr) {
            node->m_parent = parent;
            node->m_which_child = static_cast<detail::child>(which_child);
        }

        if(parent == nullptr) {
            m_root = node;
            return;
        }

        parent->m_children[which_child] = node;

        /* Since the subtree is now shorter, begin adjusting parent balances
         * and performing any needed rotations */
        do{

            /* Move up the tree and adjust the balance
             * Capture the parent and which_child values for the next
             * iteration before any rotations occur */
            node = parent;
            int old_balance = node->m_balance;
            int new_balance = old_balance - detail::child2balance[which_child];

            parent = node->m_parent;
            which_child = node->m_which_child;

            /* If a node was in perfect balance but isn't anymore then
             * we can stop, since the height didn't change above this point
             * due to a deletion. */
            if(old_balance == 0) {
                node->m_balance = new_balance;
                break;
            }

            /* If the new balance is zero, we don't need to rotate
             * else
             * need a rotation to fix the balance.
             * If the rotation doesn't change the height
             * of the sub-tree we have finished adjusting.  */
            if(new_balance == 0) {
                node->m_balance = new_balance;
            }
            else if (!rotation(node, new_balance)) {
                break;
            }
        } while (parent != nullptr);

    }

    // delete a node from the AVL tree located at the 'where' insertion_point
    void remove_at(const avltree<Tp, Dp>::insertion_point& where) {

        detail::node<Tp>* to_rm = where.m_parent;

        remove_node(to_rm);

    }

    // return the lowest valued element in the tree or nullptr
    Tp* first() const {

        detail::node<Tp>* node = first_node();

        if(node != nullptr) {
            return &node->m_data;
        }

        return nullptr;
    }

    // return the lowest valued node in the tree (nullptr if it doesn't exist)
    // (for internal use)
    detail::node<Tp>* first_node() const {

        detail::node<Tp>* node;
        detail::node<Tp>* prev = nullptr;

        for(node = m_root; node != nullptr; node = node->m_children[detail::child::LEFT]) {
            prev = node;
        }

        return prev;
    }


    // return the highest valued node in the tree or nullptr
    Tp* last() const {

        detail::node<Tp>* node = last_node();

        if(node != nullptr) {
            return &node->m_data;
        }

        return nullptr;
    }

    // return the highest valued node in the tree (nullptr if it doesn't exist)
    // (for internal use)
    detail::node<Tp>* last_node() const {

        detail::node<Tp>* node;
        detail::node<Tp>* prev = nullptr;

        for(node = m_root; node != nullptr; node = node->m_children[detail::child::RIGHT]) {
            prev = node;
        }

        return prev;
    }

    // access the node immediately before or after an insertion point
    Tp* nearest(const insertion_point& where, direction dir) {

        if(where == insertion_point::null) {
            return nullptr;
        }

        detail::node<Tp>* node = where.m_parent;
        detail::child which_child = where.m_child;


        Tp* data = &node->m_data;

        if(static_cast<int>(which_child) != static_cast<int>(dir)) {
            return data;
        }

        return walk(node, dir);
    }

    //XXX we should probably rely on iterators for this, rather
    //than jumping from Tp* to node*
    Tp* next(Tp* elem) {

        detail::node<Tp>* node = data_to_node(elem);

        return walk(node, avl::AFTER);
    }

    //XXX we should probably rely on iterators for this, rather
    //than jumping from Tp* to node*
    Tp* prev(Tp* elem) {

        detail::node<Tp>* node = data_to_node(elem);

        return walk(node, avl::BEFORE);
    }

    // walk from one node to the previous valued one (i.e. an infix walk
    // towards the left).
    Tp* walk(detail::node<Tp>* node, int left) {

        int right = 1 - left;

        // nowhere to walk if the tree is empty
        if(node == nullptr)
            return nullptr;

        // visit the previous valued node. Two possibilities:
        //
        // if this node has a left child, go down one left, 
        // then all the way to the right
        if(node->m_children[left] != nullptr) {
            for(node = node->m_children[left]; 
                node->m_children[right] != nullptr;
                node = node->m_children[right])
                ;
        }
        else{
            // otherwise, return left children as far as we can
            for(;;) {
                int was_child = node->m_which_child;
                node = node->m_parent;

                if(node == nullptr) {
                    return nullptr;
                }

                if(was_child == right) {
                    break;
                }
            }
        }

        return &node->m_data;

    }





    // search for the node that contains "value" using a binary tree search
    //
    // return value:
    //      nullptr: the value is not in the tree
    //              *where (if not nullptr) is set to indicate the insertion point
    //      Tp* of the found node
    Tp* find(const Tp& value, insertion_point& where) const {

        detail::node<Tp>* node = find_node(value, where);

        if(node != nullptr) {
            // value found -> invalidate where
            where = insertion_point::null;
            return &node->m_data;
        }

        return nullptr;
    }

    // search for the node that contains "value" using a binary tree search
    //
    // return value:
    //      nullptr: the value is not in the tree
    //              "where" is set to indicate the insertion point where the value
    //              would be inserted
    //      node<Tp>* of the found node
    //              "where" is set to the found node so that it can be passed to
    //              other internal functions instead of constructing a new 
    //              insertion_point from the return value
    detail::node<Tp>* find_node(const Tp& value, insertion_point& where) const {

        detail::node<Tp>* node;
        detail::node<Tp>* prev = nullptr;
        int child = 0;

        for(node = m_root; node != nullptr; node = node->m_children[child]) {

            prev = node;

            int diff = value.compare(node->m_data);

            assert(-1 <= diff && diff <= 1);

            if(diff == 0) {
                where = insertion_point(node, detail::child::NONE);
                return node;
            }

            child = detail::balance2child[1 + diff];
        }

        where.m_parent = prev;
        where.m_child = static_cast<detail::child>(child);

        return nullptr;
    }


    // insert a new node into the AVL tree at the specified (from find()) place
    // newly inserted nodes are always leaf nodes in the tree, since find() searches
    // out to the leaf positions.
    Tp* insert_at(const Tp& new_data, const insertion_point& where) {

        detail::node<Tp>* parent = where.m_parent;
        detail::child which_child = where.m_child;

        ++m_num_nodes;

        // create the node and add it at the indicated position
        detail::node<Tp>* n = new detail::node<Tp>(parent, which_child, new_data);

//        std::cout << " *** created avlnode: " << n << " *** \n";

        Tp* ret = &n->m_data;
        
        if(parent != nullptr) {
            assert(parent->m_children[which_child] == nullptr);
            parent->m_children[which_child] = n;
        }
        else {
            assert(m_root == nullptr);
            m_root = n;
        }

        int old_balance;
        int new_balance;

        // now, back up the tree modifying the balance of all nodes above
        // the insertion point.
        for(;;) {

            n = parent;

            if(n == nullptr) {
                return ret;
            }

            // compute the new balance
            old_balance = n->m_balance;
            new_balance = old_balance + detail::child2balance[which_child];

            // if we introduced equal balance, we are done
            if(new_balance == 0) {
                n->m_balance = 0;
                return ret;
            }

            // if both old and new are not zero, we went from -1 to -2 balance,
            // do a rotation
            if(old_balance != 0) {
                break;
            }

            n->m_balance = new_balance;
            parent = n->m_parent;
            which_child = n->m_which_child;
        }

        rotation(n, new_balance);

        return ret;

    }

    bool rotation(detail::node<Tp>* node, int balance) {

        int left = !(balance < 0);
        int right = 1 - left;
        int left_heavy = balance >> 1;
        int right_heavy = -left_heavy;

        detail::node<Tp>* parent = node->m_parent;
        detail::node<Tp>* child = node->m_children[left];

        auto which_child = node->m_which_child;
        int child_balance = child->m_balance;

        // case 1: node is overly left heavy, the left child is balanced
        // or also left heavy. We detect this situation by noting that the
        // child's balance is not right_heavy
        if(child_balance != right_heavy) {
            // if child used to be left heavy (now balanced) we reduced
            // the height of this sub-tree -- used in return...; below
            child_balance += right_heavy; 

            // move cright to be node's left child
            detail::node<Tp>* cright = child->m_children[right];
            node->m_children[left] = cright;

            if(cright != nullptr) {
                cright->m_parent = node;
                cright->m_which_child = static_cast<detail::child>(left);
            }

            // move node to be child's right child
            child->m_children[right] = node;
            node->m_balance = -child_balance;
            node->m_parent = child;
            node->m_which_child = static_cast<detail::child>(right);

            // update the pointer into this subtree
            child->m_balance = child_balance;
            child->m_which_child = which_child;
            child->m_parent = parent;

            if(parent != nullptr) {
                parent->m_children[which_child] = child;
            }
            else {
                m_root = child;
            }

            return (child_balance == 0);
        }

        // case 2: node is left heavy, but child is right heavy
        detail::node<Tp>* gchild = child->m_children[right];
        detail::node<Tp>* gleft = gchild->m_children[left];
        detail::node<Tp>* gright = gchild->m_children[right];

        // move gright to left child of node
        node->m_children[left] = gright;
        if(gright != nullptr) {
            gright->m_parent = node;
            gright->m_which_child = static_cast<detail::child>(left);
        }

        // move gleft to right child of node
        child->m_children[right] = gleft;
        if(gleft != nullptr) {
            gleft->m_parent = child;
            gleft->m_which_child = static_cast<detail::child>(right);
        }

        // move child to left child of gchild
        balance = gchild->m_balance;
        gchild->m_children[left] = child;
        child->m_balance = (balance == right_heavy ? left_heavy : 0);
        child->m_parent = gchild;
        child->m_which_child = static_cast<detail::child>(left);

        // move node to right child of gchild
        gchild->m_children[right] = node;
        node->m_balance = (balance == left_heavy ? right_heavy : 0);
        node->m_parent = gchild;
        node->m_which_child = static_cast<detail::child>(right);

        // fixup parent of all this to point to gchild
        gchild->m_balance = 0;
        gchild->m_parent = parent;
        gchild->m_which_child = which_child;

        if(parent != nullptr) {
            parent->m_children[which_child] = gchild;
        }
        else {
            m_root = gchild;
        }

        // the new tree is always shorter
        return true;
    }


    // insert new_data in the given "direction", either after or before the data "here"
    //
    // Insertions can only be done at empty leaf points in the tree, therefore
    // if the given child of the node is already present, we move to either the
    // AVL_PREV or AVL_NEXT and reverse the insertion direction. Since every other
    // node in the tree is a leaf, this always works.
    Tp* insert_here(const Tp& new_data, detail::node<Tp>* here, direction dir) {

        assert(here != nullptr);

        int child = static_cast<int>(dir);

        // if the corresponding child of node is not nullptr, go to the neighboring
        // node and reverse the insertion direction
        detail::node<Tp>* node = data_to_node(&here->m_data);

        if(node->m_children[child] != nullptr) {
            node = node->m_children[child];
            child = 1 - child;

            while(node->m_children[child] != nullptr) {
                node = node->m_children[child];
            }
        }

        assert(node->m_children[child] == nullptr);

        return insert_at(new_data, insertion_point(node, static_cast<detail::child>(child)));
    }

    detail::node<Tp>* destroy_nodes(index& current) {

        detail::node<Tp>* node = nullptr;
        detail::node<Tp>* parent = nullptr;
        int child;

        /* initial calls go to the first node or it's right descendant */
        if(current == index::null) {
            detail::node<Tp>* first = this->first_node();

            if(first == nullptr) {
                // remember that the tree is done for future iterations
                current.m_parent = nullptr;
                current.m_child = static_cast<detail::child>(1);
                return nullptr;
            }

            node = first;
            parent = node->m_parent;

            goto check_right_side;
        }

        /* if there is no parent to return to, we are done */
        parent = current.m_parent;

        if(parent == nullptr) {
            if(m_root != nullptr) {
                assert(m_num_nodes == 1);
                m_root = nullptr;
                m_num_nodes = 0;
            }

            return nullptr;
        }

        /* remove the child pointer we just visited from the parent and tree */
        child = current.m_child;
        parent->m_children[child] = nullptr;
        assert(m_num_nodes > 1);
        --m_num_nodes;

        /* if we just did a right child or there isn't one, go up to the parent */
        if(child == 1 || parent->m_children[1] == nullptr) {
            node = parent;
            parent = parent->m_parent;
            goto done;
        }

        /* do parent's right child, then leftmost descendant */
        node = parent->m_children[1];
        while(node->m_children[0] != nullptr) {
            parent = node;
            node = node->m_children[0];
        }

        /* if we made it here, we moved to a left child. It may have
         * one child on the right (when balance == +1) */
check_right_side:

        if(node->m_children[1] != nullptr) {
            assert(node->m_balance == 1);
            parent = node;
            node = node->m_children[1];
            assert(node->m_children[0] == nullptr &&
                   node->m_children[1] == nullptr);
        }
        else {
            assert(node->m_balance <= 0);
        }

done:
        if(parent == nullptr) {
            // remember that the tree is done for future iterations
            current.m_parent = nullptr;
            current.m_child = static_cast<detail::child>(1);

            assert(node == m_root);
        }
        else {
            current.m_parent = parent;
            current.m_child = node->m_which_child;
        }

        return node;

    }


#ifdef __AVL_DEBUG__
    void preorder(std::ostream& os, detail::node<Tp>* node) const {

        static int nullcount = 0;

        if(node != nullptr) {
            os << node->m_data.id() << "[label=\"" << node->m_data 
               << "\\nbal: " << node->m_balance
               << "\\nchild: " << node->m_which_child
               << "\"];\n";

            if(node->m_children[detail::child::LEFT] != nullptr) {
                os << node->m_data.id() << " -> "
                   << node->m_children[detail::child::LEFT]->m_data.id() << ";\n";
            }
            else {
                os << "null" << nullcount << "[shape=point];\n"
                   << node->m_data.id() << " -> null" << nullcount << ";\n";
                ++nullcount;
            }

            if(node->m_children[detail::child::RIGHT] != nullptr) {
                os << node->m_data.id() << " -> "
                   << node->m_children[detail::child::RIGHT]->m_data.id() << ";\n";
            }
            else {
                os << "null" << nullcount << "[shape=point];\n"
                   << node->m_data.id() << " -> null" << nullcount << ";\n";
                ++nullcount;
            }

            preorder(os, node->m_children[detail::child::LEFT]);
            preorder(os, node->m_children[detail::child::RIGHT]);
        }
    }

    std::string dot() const {

        std::stringstream ss;

        ss << "digraph {\n";
        preorder(ss, m_root);
        ss << "}\n";
        return ss.str();
    }

    void display() const {

        //std::cout << dot() << "\n";
        //std::cout << std::is_standard_layout<detail::node<Tp>>::value << "\n";
        //std::cout << std::is_standard_layout<Tp>::value << "\n";

        std::ofstream out("/tmp/out.gv");
        out << dot() << std::endl;
        out.close();
        system("dot -Tpng /tmp/out.gv -o /tmp/out.png");
        system("eog /tmp/out.png");
    }
#endif


    static Tp* node_to_data(detail::node<Tp>* n) {
        auto off = offsetof(detail::node<Tp>, detail::node<Tp>::m_data);
        return (Tp*) ((uintptr_t)n + off);
    }

    static detail::node<Tp>* data_to_node(Tp* d) {
        auto off = offsetof(detail::node<Tp>, detail::node<Tp>::m_data);
        return (detail::node<Tp>*) ((uintptr_t) d - off);
    }


private:

    detail::node<Tp>* m_root = nullptr;
    uint32_t m_num_nodes = 0;
    Dp m_deleter;

};

template <typename Tp, typename Dp>
const typename avltree<Tp, Dp>::insertion_point avltree<Tp, Dp>::insertion_point::null;


} // namespace avl



#endif /* __AVL_HPP__ */
