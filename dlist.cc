#include "dlist.h"
#include <cstdio>
#include <cassert>

DLink* DList::Pop()
    {
    DLink*    result = nullptr;
    if(count)
        result = Remove(head);
    return result;
    }

DLink* DList::Remove(DLink* link)
    {
    fprintf(stderr, "DList::Remove %p\n", link);
    assert(count > 0);
    assert(Contains(link));
    assert(link->next != nullptr && link->previous != nullptr);
    if(link->next == link)  // if last link in circular queue
        {
        assert(count == 1);
        head    = nullptr;
        }
    else
        {
        if(head == link)             // if this is head link in queue
            head    = link->next;
        link->next->previous = link->previous;
        link->previous->next = link->next;
        }
    link->next = link->previous = nullptr;
    --count;
    return link;
    }

void DList::Push(DLink* newTail)
    {
//    fprintf(stderr, "Push %p\n", newTail);
    assert(newTail->next == nullptr); // should not be on any list
    assert(newTail->previous == nullptr);
    if(head == nullptr)
        {
        assert(count == 0);
        head = newTail;
        newTail->next = newTail->previous = newTail;
        }
    else
        {
        auto oldTail            = head->previous;
        newTail->next           = head;
        newTail->previous       = oldTail;
        oldTail->next           = newTail;
        head->previous          = newTail;
        }
    ++count;
    assert(Contains(newTail));
    }

