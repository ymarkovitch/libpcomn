/*-*- tab-width:4;c-basic-offset:4;indent-tabs-mode:nil -*-*/
/*******************************************************************************
 FILE         :   dotnet_helpers.cs
 DESCRIPTION  :   Utility helpers for .NET

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Jul 2015
*******************************************************************************/
using System ;
using System.Runtime ;
using System.Diagnostics ;
using System.Reflection ;

namespace PComn {

    /** *************************************************************************
     * <summary>A holder for helper functions and extension methods</summary>
     ****************************************************************************/
    public static class Helpers {

        /// <summary>
        ///   Throw NotImplementedException
        /// </summary>
        public static void Unimplemented(string methodName)
        {
            throw new NotImplementedException(String.Format("'{0}' method is not implemented.", methodName)) ;
        }

        /// <summary>
        ///   Throw NotSupportedException
        /// </summary>
        public static void Unsupported(string methodName)
        {
            throw new NotSupportedException(String.Format("'{0}' method is not supported.", methodName)) ;
        }

        /// <summary>
        ///   Throw ArgumentNullException if the value is null
        /// </summary>
        public static T EnsureNotNull<T>(this T value, string message) where T : class
        {
            if (value == null)
                throw new ArgumentNullException(message) ;
            return value ;
        }

        /// <summary>
        ///   Return true if "this" is an empty string or null
        /// </summary>
        public static bool IsEmpty(this string str)
        {
            return str == null || str.Length == 0 ;
        }
    }
} // end of namespace PComn
