/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __TEST_JOURNAL_H
#define __TEST_JOURNAL_H
/*******************************************************************************
 FILE         :   test_journal.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Journal test classes

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Nov 2008
*******************************************************************************/
#include <pcomn_journal/journal.h>

#include <pcomn_omanip.h>
#include <pcomn_diag.h>

#include <iostream>
#include <map>
#include <mutex>

namespace pj = pcomn::jrn ;

enum StringMapOpcode {
   SM_OP_ADD = 1,
   SM_OP_DEL = 2,
   SM_OP_CLR = 3
} ;

#define PCOMN_APPLY_STRINGMAP_JOURNAL_OP(MACRO) \
    MACRO(ADD)                                \
    MACRO(DEL)                                \
    MACRO(CLR)

template<StringMapOpcode opcode> struct StringMapOp ;


class AbstractExceptionContainer {
   public:
      virtual ~AbstractExceptionContainer() {}
      virtual void raise() const = 0 ;
   protected:
      AbstractExceptionContainer(std::exception *x) :
         _x(x)
      {}

      const std::exception &x() const { return *_x ; }

   private:
      pcomn::PTSafePtr<std::exception> _x ;
} ;

template<class X>
class ExceptionContainer : public AbstractExceptionContainer {
      typedef AbstractExceptionContainer ancestor ;
   public:
      ExceptionContainer(const X &x) :
         AbstractExceptionContainer(new X(x))
      {}

      const X &x() const { return *static_cast<const X *>(&ancestor::x()) ; }
      void raise() const { throw X(x()) ; }
} ;

/*******************************************************************************
                     class JournallableStringMap
*******************************************************************************/
class JournallableStringMap : public pj::Journallable {
      typedef pj::Journallable ancestor ;
   public:
      // All our operations _are_ our friends!
#define PCOMN_DECL_FRIEND_OP_(op) friend struct StringMapOp<SM_OP_##op> ;
      PCOMN_APPLY_STRINGMAP_JOURNAL_OP(PCOMN_DECL_FRIEND_OP_)
#undef PCOMN_DECL_FRIEND_OP_

      typedef std::map<std::string, std::string> string_map ;

      enum Stage {
         S_START_CHECKPOINT,
         S_SAVE_CHECKPOINT,
         S_FINISH_CHECKPOINT,
         S_RESTORE_CHECKPOINT
      } ;

      JournallableStringMap()
      {}

      explicit JournallableStringMap(const string_map &initval) :
         _data(initval)
      {}

      template<typename InputIterator>
      JournallableStringMap(InputIterator begin, InputIterator end) :
         _data(begin, end)
      {}

      string_map data() const { return _data ; }

      size_t size() const { return _data.size() ; }

      JournallableStringMap &insert(const std::string &key, const std::string &value) ;

      JournallableStringMap &insert(const std::string &key) ;

      JournallableStringMap &erase(const std::string &key) ;

      JournallableStringMap &clear() ;

      template<class X>
      void set_exception(Stage stage, const X &x)
      {
         _exception = new ExceptionContainer<X>(x) ;
         _xstage = stage ;
      }

      void check_exception(Stage stage) const
      {
         if (stage == _xstage && _exception)
         {
            pcomn::PTSafePtr<AbstractExceptionContainer> x ;
            pcomn_swap(x, _exception) ;
            x->raise() ;
         }
      }

      static JournallableStringMap *from_stream(std::istream &input)
      {
         string_map data ;
         for (std::string key, line ; std::getline(input, key) ; line.clear())
         {
            std::getline(input, line) ;
            data[key] = line ;
         }
         return new JournallableStringMap(data) ;
      }

      static const pj::magic_t MAGIC ;

   protected:
      std::ostream &debug_print(std::ostream &os) const
      {
         return ancestor::debug_print(os) << ":size=" << size() ;
      }

      pj::operation_ptr create_operation(pj::opcode_t opcode, pj::opversion_t version) const ;

      std::string readable_opname(pj::opcode_t code, pj::opversion_t version) const
      {
         switch (code)
         {
            #define RET_OP_NAME_(op) case SM_OP_##op : return version == 2 ? #op "2" : #op ;
            PCOMN_APPLY_STRINGMAP_JOURNAL_OP(RET_OP_NAME_) ;
            #undef RET_OP_NAME_
         }
         return ancestor::readable_opname(code, version) ;
      }

      static std::string &strip_lf(std::string &line)
      {
         const size_t sz = line.size() - 1 ;
         if (line[sz] == '\n')
            line.resize(sz) ;
         return line ;
      }

      void restore_checkpoint(pcomn::binary_ibufstream &checkpoint, size_t data_size)
      {
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Restoring checkpoint of " << *this) ;
         PCOMN_USE(data_size) ;
         PCOMN_VERIFY(state() == ST_RESTORING) ;

         check_exception(S_RESTORE_CHECKPOINT) ;

         string_map restored ;
         for (std::string key, value ; !(key = pcomn::readline(checkpoint)).empty() ; )
         {
            value = pcomn::readline(checkpoint) ;
            pcomn_swap(restored[strip_lf(key)], strip_lf(value)) ;
         }
         pcomn_swap(_data, restored) ;
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Checkpoint of " << *this << " restored OK") ;
      }

      void start_checkpoint()
      {
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Starting checkpoint of " << *this) ;
         PCOMN_VERIFY(state() == ST_CHECKPOINT) ;

         check_exception(S_START_CHECKPOINT) ;
         {
            guard lock (_mutex) ;
            NOXCHECK(_snapshot.empty()) ;
            _snapshot = _data ;
         }
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Checkpoint of " << *this << " started") ;
      }

      void save_checkpoint(pcomn::binary_obufstream &checkpoint)
      {
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Saving checkpoint of " << *this << ", "
                 << _snapshot.size() << " items") ;

         PCOMN_VERIFY(state() == ST_CHECKPOINT) ;

         check_exception(S_SAVE_CHECKPOINT) ;

         for (string_map::const_iterator i (_snapshot.begin()) ; i != _snapshot.end() ; ++i)
         {
            checkpoint.write(i->first) ;
            checkpoint.put('\n') ;
            checkpoint.write(i->second) ;
            checkpoint.put('\n') ;
         }

         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Checkpoint of " << *this << " saved") ;
      }

      void finish_checkpoint() throw()
      {
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Finishing checkpoint of " << *this) ;
         check_exception(S_FINISH_CHECKPOINT) ;
         PCOMN_VERIFY(state() == ST_CHECKPOINT) ;

         {
            guard lock (_mutex) ;
            _snapshot.clear() ;
         }
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Checkpoint of " << *this << " finished") ;
      }

      bool fill_user_magic(pj::magic_t &magic) const
      {
         TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Filling user magic '" << MAGIC << "' for " << *this) ;
         magic = MAGIC ;
         return true ;
      }

   private:
      std::mutex  _mutex ;
      string_map  _data ;
      string_map  _snapshot ;

      mutable pcomn::PTSafePtr<AbstractExceptionContainer> _exception ;
      Stage                                        _xstage ; /* Where to throw exception */

      typedef std::lock_guard<std::mutex> guard ;
} ;

const pj::magic_t JournallableStringMap::MAGIC = {{ '@', 'J', 'S', '_', 'm', 'a', 'p', '\0' }} ;

/*******************************************************************************
 Operations
*******************************************************************************/
template<> class StringMapOp<SM_OP_ADD> : public pj::TargetOperation<JournallableStringMap> {
      typedef pj::TargetOperation<JournallableStringMap> ancestor ;
   public:
      StringMapOp(JournallableStringMap &target, const std::string &key, const std::string &data) :
         ancestor(target, SM_OP_ADD),
         _key(key),
         _data(data)
      {}

      StringMapOp(JournallableStringMap &target, const std::string &key) :
         ancestor(target, SM_OP_ADD, 2),
         _key(key)
      {}

      StringMapOp(const JournallableStringMap &target, pj::opversion_t opversion) :
         ancestor(target, SM_OP_ADD, opversion)
      {
         PCOMN_VERIFY((pcomn::one_of<1, 2>::is(opversion))) ;
      }

   protected:
      void do_apply(JournallableStringMap &target) const
      {
         switch (version())
         {
            case 1: target._data[_key] = _data ;
               break ;

            case 2: target._data[_key] =
               pcomn::str::to_upper(_key).append(1, '-').append(pcomn::str::to_upper(_key)) ;
               break ;

            default:
               PCOMN_FAIL("Invalid OP_ADD version!") ;
         }
      }
      void do_save(pcomn::binary_obufstream &os) const
      {
         os.write(_key) ;
         if (version() == 1)
         {
            os.put('\n') ;
            os.write(_data) ;
         }
      }
      void do_restore(const void *buf, size_t sz)
      {
         if (!sz)
            _key = _data = std::string() ;

         else if (version() == 2)
         {
            _data = std::string() ;
            std::string((const char *)buf, (const char *)buf + sz).swap(_key) ;
         }
         else
         {
            const char *endkey = (const char *)memchr(buf, '\n', sz) ;
            if (!endkey)
               endkey = (const char *)buf + sz - 1 ;
            std::string key ((const char *)buf, endkey) ;
            std::string value (endkey + 1, (const char *)buf + sz) ;
            pcomn_swap(_key, key) ;
            pcomn_swap(_data, value) ;
         }
      }

   private:
      std::string _key ;
      std::string _data ;
} ;

template<> class StringMapOp<SM_OP_DEL> : public pj::TargetOperation<JournallableStringMap> {
      typedef pj::TargetOperation<JournallableStringMap> ancestor ;
   public:
      StringMapOp(JournallableStringMap &target, const std::string &key) :
         ancestor(target, SM_OP_DEL),
         _key(key)
      {}

      StringMapOp(const JournallableStringMap &target, pj::opversion_t opversion) :
         ancestor(target, SM_OP_DEL, opversion)
      {
         PCOMN_VERIFY(opversion = 1) ;
      }

   protected:
      void do_apply(JournallableStringMap &target) const
      {
         target._data.erase(_key) ;
      }
      void do_save(pcomn::binary_obufstream &os) const
      {
         os.write(_key) ;
      }
      void do_restore(const void *buf, size_t sz)
      {
         std::string key ((const char *)buf, (const char *)buf + sz) ;
         pcomn_swap(_key, key) ;
      }
   private:
      std::string _key ;
} ;

template<>
class StringMapOp<SM_OP_CLR> : public pj::TargetOperation<JournallableStringMap, pj::BodylessOperation> {
      typedef pj::TargetOperation<JournallableStringMap, pj::BodylessOperation> ancestor ;
  public:
      StringMapOp(const JournallableStringMap &target, pj::opversion_t opversion = 1) :
         ancestor(target, SM_OP_CLR, opversion)
      {
         PCOMN_VERIFY(opversion = 1) ;
      }
  protected:
      void do_apply(JournallableStringMap &target) const
      {
         target._data.clear() ;
      }
} ;

/*******************************************************************************
 JournallableStringMap
*******************************************************************************/
pj::operation_ptr JournallableStringMap::create_operation(pj::opcode_t opcode,
                                                          pj::opversion_t version) const
{
   TRACEPX(PCOMN_Test, DBGL_ALWAYS, "Create operation " << opcode << " version " << version) ;

   #define NEW_OP(code) case SM_OP_##code: return pj::operation_ptr(new StringMapOp<SM_OP_##code>(*this, version)) ;
   switch (opcode)
   {
      PCOMN_APPLY_STRINGMAP_JOURNAL_OP(NEW_OP) ;
      default: pcomn::throw_exception<pj::opcode_error>(opcode, version) ;
   }
   return pj::operation_ptr() ;
}

JournallableStringMap &JournallableStringMap::insert(const std::string &key, const std::string &value)
{
   apply(StringMapOp<SM_OP_ADD>(*this, key, value)) ;
   return *this ;
}

JournallableStringMap &JournallableStringMap::insert(const std::string &key)
{
   apply(StringMapOp<SM_OP_ADD>(*this, key)) ;
   return *this ;
}

JournallableStringMap &JournallableStringMap::erase(const std::string &key)
{
   apply(StringMapOp<SM_OP_DEL>(*this, key)) ;
   return *this ;
}

JournallableStringMap &JournallableStringMap::clear()
{
   apply(StringMapOp<SM_OP_CLR>(*this)) ;
   return *this ;
}

#endif /* __TEST_JOURNAL_H */
