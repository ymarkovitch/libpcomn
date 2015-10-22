/*-*- tab-width:2;c-basic-offset:2;indent-tabs-mode:nil -*-*/
/*******************************************************************************
 FILE         :   dotnet_swigwrapper.cs
 DESCRIPTION  :   The library of helper functions and classes for SWIG-generated
                  .NET wrappers

                  Module initialization and finalization:
                    - WrapperModule
                    - WrapperHierarchyMapper

                  Diagnostics and tracing:
                    - DiagGroup
                    - Tracer

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Jul 2015
*******************************************************************************/
using System ;
using System.Runtime ;
using System.Runtime.InteropServices ;
using System.Diagnostics ;
using System.Reflection ;
using System.Collections ;
using System.Linq ;

namespace PComn {

  /***************************************************************************
   * <summary>
   * The internal name of the wrapper class (i.e. the name of the wrapped class)
   * </summary>
   **************************************************************************/
  public class InternalNameAttribute : Attribute {
    public readonly string Value ;

    public InternalNameAttribute(string name) { Value = name ; }
  }

  /***************************************************************************
   * <summary>
   * The internal name of the wrapper class (i.e. the name of the wrapped class)
   * </summary>
   **************************************************************************/
  class InstanceConstructor {
    public InstanceConstructor(Type type, params Type[] constructorArgs)
    {
      ctorInfo = type.GetConstructor(ctorBinding, null, constructorArgs, null) ;
    }
    public object Construct(params object[] args) { return ctorInfo.Invoke(args) ; }

    public Type InstanceType { get { return ctorInfo.DeclaringType ; } }

    const BindingFlags  ctorBinding = BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public ;
    ConstructorInfo     ctorInfo ;
  }

  /***************************************************************************
   * <summary>
   * Maps native class ID (whatever it means) to corresponding wrapper
   * class constructor
   * </summary>
   **************************************************************************/
  class WrapperHierarchyMapper {

    public delegate string ClassNameGetter(IntPtr classId) ;
    public delegate IntPtr BaseClassIdGetter(IntPtr classId) ;

    public WrapperHierarchyMapper(ClassNameGetter getClassName,
                                  BaseClassIdGetter getBaseClassId)
    {
      this.getClassName = getClassName.EnsureNotNull("getClassName") ;
      this.getBaseClassId = getBaseClassId.EnsureNotNull("getBaseClassId") ;
    }

    public void AppendConstructor(Type wrapperType, Type[] ctorArgTypes)
    {
      string name = (Attribute.
                     GetCustomAttribute(wrapperType, typeof(InternalNameAttribute))
                     as InternalNameAttribute).Value ;
      AppendConstructor(name, wrapperType, ctorArgTypes) ;
    }

    public void AppendConstructor(string internalName, Type wrapperType, Type[] ctorArgTypes)
    {
      writeTraceLine("Registering {0}, wrapper={1}", internalName, wrapperType) ;
      _typesByName[internalName] = new InstanceConstructor(wrapperType, ctorArgTypes) ;
    }

    public void AppendAssemblyTypes(Assembly assembly, Type[] ctorArgTypes, Type basetype)
    {
      foreach (Type type in assembly.GetTypes())
        if ((type.IsSubclassOf(basetype) ||
             basetype.IsInterface && type.GetInterface(basetype.FullName) != null) &&
            Attribute.GetCustomAttribute(type, typeof(InternalNameAttribute)) != null)
          AppendConstructor(type, ctorArgTypes) ;
    }

    public InstanceConstructor GetConstructor(IntPtr classID)
    {
      Debug.Assert((int)classID != 0, "Class ID is NULL!") ;

      InstanceConstructor result = _typesByClassID[classID] as InstanceConstructor ;
      if (result == null)
      {
        // This class was never constructed before. If we know it by name,
        // register immediately; if not, walk back along the hierarchy until
        // the first known class will be found
        result = _typesByName[getClassName(classID)] as InstanceConstructor ;
        if (result == null)
          result = GetConstructor(getBaseClassId(classID)) ;
        _typesByClassID[classID] = result ;
      }
      return result ;
    }

    /***************************************************************************
    private
    ***************************************************************************/
    private readonly ClassNameGetter    getClassName ;
    private readonly BaseClassIdGetter  getBaseClassId ;

    private readonly Hashtable _typesByName = new Hashtable(61) ;
    private readonly Hashtable _typesByClassID = new Hashtable(97) ;

    [Conditional("TRACE")]
    static void writeTraceLine(string format, params object[] args)
    {
    }

  }

} // end of namespace PComn
