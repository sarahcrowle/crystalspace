/*
    Copyright (C) 2012 by Frank Richter

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
    
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CS_UTIL_INVASIVELIST_H__
#define __CS_UTIL_INVASIVELIST_H__

/**\file
 * "Invasive" double-linked list.
 */

namespace CS
{
  namespace Container
  {
    /**
     * "Invasive" double-linked list.
     * The list is invasive as all elements in the list have to be derived from,
     * or have a member of, the Element class.
     * However, this allows for more flexibility in the data structures you can
     * construct - elements from the double-linked list can also be members of
     * other structures simultaneously (e.g. hashes, or other linked list).
     * \tparam Tag is an arbitrary type to distinguish different list types.
     */
    template<typename Tag>
    class InvasiveList
    {
    public:
      class Element
      {
        friend class InvasiveList<Tag>;
        
        Element* next;
        Element* prev;
      public:
        Element() : next (nullptr), prev (nullptr) {}
      };
      
      InvasiveList () : head (nullptr), tail (nullptr) {}

      /// Push an element to the end of the list.
      void PushBack (Element* item)
      {
        item->prev = tail;
        if (tail) tail->next = item;
        tail = item;
        if (!head) head = item;
      }
      
      /**
       * Remove and return the first element.
       * \returns Previous first element; \c nullptr if list was empty.
       */
      Element* PopFront()
      {
        Element* front (head);
        if (front) Delete (front);
        return front;
      }

      /// Delete an element from the list.
      void Delete (Element* item)
      {
        if (head == item) head = item->next;
        if (tail == item) tail = item->prev;
        if (item->prev) item->prev->next = item->next;
        if (item->next) item->next->prev = item->prev;
        item->prev = nullptr;
        item->next = nullptr;
      }
      
      /// Return if the list is empty.
      bool IsEmpty() const
      {
        return !head;
      }
    private:
      Element* head;
      Element* tail;
    };
  } // namespace Container
} // namespace CS

#endif // __CSUTIL_INVASIVELIST_H__
