#ifndef DLIST_H
#define DLIST_H
/* DList - for very simple "intrusive" doubly-linked list.
 *
 * Nearly zero space cost apart from the two link pointers.
 * Adding is O(1), removing is O(1), searching is O(n).
 */

struct DLink
    {
    DLink*  next        = nullptr;
    DLink*  previous    = nullptr;
    };

class DList
    {
    DLink*  head;
    int     count;
public:
    DList() : head(nullptr), count(0) {}

    DLink*  Head() { return head; }
    void    Push(DLink* link);
    DLink*  Remove(DLink* link);
    DLink*  Pop();
    int     Count() { return count; }
    bool    Contains(DLink* This) { return true; }
    };

//template<typename T>
//class DList<T> : DList

#endif /* DLIST_H */
