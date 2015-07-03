//------------------------------------------------------------------------
// ^FILE: fifolist.h - generic FIFO list classes
//
// ^DESCRIPTION:
//    This file defines a generic FIFO linked list class and two types
//    of iterators for the list.  The first iterator is just your basic
//    run-of-the-mill iterator.  The second iterator treats the list
//    as if it were an array and allows you to index into the list.
//
//    Once these generic classes are declared, macros are defined to allow
//    the programmer to declare lists (and iterators) that contain a
//    particular type of item.  On systems where your C++ compiler supports
//    templates, templates are used, otherwise we "fake it".
//
//    The macro defined is named DECLARE_FIFO_LIST and is used as follows:
//
//       DECLARE_FIFO_LIST(Name, Type);
//
//    This declares a type named "Name" that is a list of pointers to
//    items of type "Type".  Also, the types "NameIter" and "NameArray"
//    are declared as the iterators for this type of list.
//
// ^HISTORY:
//    03/21/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef _fifolist_h
#define _fifolist_h

#ifndef name2
#  define  name2(x,y) x##y
#endif

   // GenericFifoList - a FIFO linked list of void * pointers
   //
class  GenericFifoList {
private:

protected:
   // Need to define what a "node" in the list looks like
   struct GenericFifoListNode {
      GenericFifoListNode * next;
      void * contents;

      GenericFifoListNode(GenericFifoListNode * nd =0, void * val =0)
         : next(nd), contents(val) {}
   } ;

   GenericFifoListNode * head;
   GenericFifoListNode * tail;
   unsigned  num_items ;
   bool mod ;
   bool del_items ;

   GenericFifoList() :
      head(),
      tail(),
      num_items(0),
      mod(false),
      del_items(false)
   {}

   // Remove the first item from the list
   void *
   remove();

   // Add an item to the end of the list
   void
   add(void * item);

public:
   virtual  ~GenericFifoList() = 0 ;

   // Was the list modified since the last time we checked?
   bool
   modified() { return mod ? (mod = false, true) : false ; }

   // Is the list empty?
   bool
   is_empty() const { return !num_items ; }

   // How many items are in the list?
   unsigned
   count() const { return  num_items; }

   // Is the list responsible for deleting the items it contains?
   bool
   self_cleaning() const { return del_items ; }

   // Tell the list who is responsible for deleting the items it contains?
   void
   self_cleaning(bool bool_val)  { del_items = bool_val ; }

   friend class GenericFifoListIter;
   friend class GenericFifoListArray;
} ;


   // GenericFifoListIter -- an iterator for a GenericFifoList
class  GenericFifoListIter {
private:
   GenericFifoList::GenericFifoListNode * current;

protected:
   GenericFifoListIter(GenericFifoList & fifo_list)
      : current(fifo_list.head) {}

   GenericFifoListIter(GenericFifoList * fifo_list)
      : current(fifo_list->head) {}

   // Return the current item in the list and advance to the next item.
   // returns NULL if at end-of-list
   //
   void *
   operator()();

public:
   virtual  ~GenericFifoListIter();

} ;


   // GenericFifoListArray -- an array-style iterator for a GenericFifoList
class  GenericFifoListArray {
private:
   GenericFifoList &  list;
   unsigned           index;
   GenericFifoList::GenericFifoListNode * current;

protected:
   GenericFifoListArray(GenericFifoList & fifo_list)
      : list(fifo_list), index(0), current(fifo_list.head) {}

   GenericFifoListArray(GenericFifoList * fifo_list)
      : list(*fifo_list), index(0), current(fifo_list->head) {}

   // How many items are in the array?
   unsigned  count() const  { return  list.count(); }

   // Return a specified item in the array.
   //   NOTE: the programmer is responsible for making sure the given index
   //         is not out of range. For this base class, NULL is returned
   //         when the index is out of range. Derived classes however
   //         dereference the value returned by this function so using
   //         an out-of-range index in one of the derived classes will
   //         cause a NULL pointer dereferencing error!
   //
   void *
   operator[](unsigned  ndx);

public:
   virtual  ~GenericFifoListArray();

} ;

template <class Type>
class CmdFifoList : public GenericFifoList {
public:
   ~CmdFifoList();

   void
   add(Type * item)  { GenericFifoList::add((void *)item); }

   Type *
   remove()  { return  (Type *) GenericFifoList::remove(); }
} ;

template <class Type>
class CmdFifoListIter : public GenericFifoListIter {
public:
   CmdFifoListIter(CmdFifoList<Type> & list) : GenericFifoListIter(list) {}
   CmdFifoListIter(CmdFifoList<Type> * list) : GenericFifoListIter(list) {}

   Type *
   operator()()  {  return  (Type *) GenericFifoListIter::operator()(); }
} ;

template <class Type>
class CmdFifoListArray : public GenericFifoListArray {
public:
   CmdFifoListArray(CmdFifoList<Type> & list) : GenericFifoListArray(list) {}
   CmdFifoListArray(CmdFifoList<Type> * list) : GenericFifoListArray(list) {}

   Type &
   operator[](unsigned  ndx)
      { return  *((Type *) GenericFifoListArray::operator[](ndx)); }
} ;

// Destructor
template <class Type>
CmdFifoList<Type>::~CmdFifoList() {
   GenericFifoListNode * nd = head;
   head = NULL;
   while (nd) {
      GenericFifoListNode * to_delete = nd;
      nd = nd->next;
      if (del_items)  delete (Type *)to_delete->contents;
      delete  to_delete;
   }
}

#define  DECLARE_FIFO_LIST(Name,Type)                   \
   typedef  CmdFifoList<Type> Name;                     \
   typedef  CmdFifoListIter<Type>  name2(Name,Iter);    \
   typedef  CmdFifoListArray<Type> name2(Name,Array)

#endif /* _fifolist_h */
