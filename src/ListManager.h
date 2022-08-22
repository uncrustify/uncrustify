/**
 * @file ListManager.h
 * Template class that manages items in a double-linked list.
 * If C++ could do it, this would just be a class that worked on an interface.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef LIST_MANAGER_H_INCLUDED
#define LIST_MANAGER_H_INCLUDED

#include "chunk.h"

/*
 * TODO: why do we provide this template class? can't we use
 * a double linked list std::deque from the standard library ?
 */
/**
 * A simple list manager for a double-linked list.
 * Class T must define 'm_next' and 'm_prev', which must be pointers to type T.
 */
template<class T>
class ListManager
{
protected:
   T *first; //! pointer to the head of list
   T *last;  //! pointer to tail of list

public:
   ListManager()
   {
      first = Chunk::NullChunkPtr;
      last  = Chunk::NullChunkPtr;
   }


   /**
    * @brief return the first element of the linked list
    *
    * @return pointer to first element or Chunk::NullChunkPtr if list is empty
    */
   T *GetHead() const
   {
      return(first);
   }


   /**
    * @brief return the last element of the linked list
    *
    * @return pointer to last element or Chunk::NullChunkPtr if list is empty
    */
   T *GetTail() const
   {
      return(last);
   }


   /**
    * @brief remove an element from a linked list
    *
    * @param[in] obj  list element to remove
    */
   void Pop(T *obj)
   {
      if (obj != Chunk::NullChunkPtr)
      {
         if (first == obj)
         {
            first = obj->m_next;
         }

         if (last == obj)
         {
            last = obj->m_prev;
         }

         if (obj->m_next != Chunk::NullChunkPtr)
         {
            obj->m_next->m_prev = obj->m_prev;
         }

         if (obj->m_prev != Chunk::NullChunkPtr)
         {
            obj->m_prev->m_next = obj->m_next;
         }
         obj->m_next = Chunk::NullChunkPtr;
         obj->m_prev = Chunk::NullChunkPtr;
      }
   }


   //! swap two elements of a list
   void Swap(T *obj1, T *obj2)
   {
      if (  obj1 != Chunk::NullChunkPtr
         && obj2 != Chunk::NullChunkPtr)
      {
         if (obj1->m_prev == obj2)
         {
            Pop(obj1);
            AddBefore(obj1, obj2);
         }
         else if (obj2->m_prev == obj1)
         {
            Pop(obj2);
            AddBefore(obj2, obj1);
         }
         else
         {
            T *m_prev1 = obj1->m_prev;
            Pop(obj1);

            T *m_prev2 = obj2->m_prev;
            Pop(obj2);

            AddAfter(obj1, m_prev2);
            AddAfter(obj2, m_prev1);
         }
      }
   }


   /**
    * @brief add a new element after a reference position in a list
    *
    * @param obj  new element to add to list
    * @param ref  chunk after which to insert new object
    */
   void AddAfter(T *obj, T *ref)
   {
      if (  obj != Chunk::NullChunkPtr
         && ref != Chunk::NullChunkPtr)
      {
         Pop(obj); // TODO: is this necessary?
         obj->m_next = ref->m_next;
         obj->m_prev = ref;

         if (ref->m_next != Chunk::NullChunkPtr)
         {
            ref->m_next->m_prev = obj;
         }
         else
         {
            last = obj;
         }
         ref->m_next = obj;
      }
   }


   /**
    * @brief add a new element before a reference position in a list
    *
    * @param obj  new element to add to list
    * @param ref  chunk before to insert new object
    */
   void AddBefore(T *obj, T *ref)
   {
      if (  obj != Chunk::NullChunkPtr
         && ref != Chunk::NullChunkPtr)
      {
         Pop(obj);
         obj->m_next = ref;
         obj->m_prev = ref->m_prev;

         if (ref->m_prev != Chunk::NullChunkPtr)
         {
            ref->m_prev->m_next = obj;
         }
         else
         {
            first = obj;
         }
         ref->m_prev = obj;
      }
   }


   /**
    * @brief add a new element to the tail of a lis
    *
    * @param obj  new element to add to the list
    */
   void AddTail(T *obj)
   {
      obj->m_next = Chunk::NullChunkPtr;
      obj->m_prev = last;

      if (last == Chunk::NullChunkPtr)
      {
         last  = obj;
         first = obj;
      }
      else
      {
         last->m_next = obj;
      }
      last = obj;
   }


   /**
    * @brief add a new element to the head of a list
    *
    * @param obj  new element to add to the list
    */
   void AddHead(T *obj)
   {
      obj->m_next = first;
      obj->m_prev = Chunk::NullChunkPtr;

      if (first == Chunk::NullChunkPtr)
      {
         last  = obj;
         first = obj;
      }
      else
      {
         first->m_prev = obj;
      }
      first = obj;
   }
};


#endif /* LIST_MANAGER_H_INCLUDED */
