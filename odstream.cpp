/***********************************************************************/
/*                                                                     */
/* odstream.cpp: Source file for stream class using OutputDebugString  */
/*                                                                     */
/*               Copyright (c) Yak! 2005 All rights reserved.          */
/*                                                                     */
/*   $URL$               */
/*  $Date$              */
/*   $Rev$                                                        */
/*                                                                     */
/***********************************************************************/

#include <sstream>

#include <windows.h>

#include "odstream.hpp"

namespace yak {

	namespace debug_yes {

		class odstringbuf : public std::stringbuf
		{
		protected:
			virtual int sync(void) {
				OutputDebugString(str().c_str());
				str("");
				return 0;
			}
		};

		odstringbuf odsbuf;
		std::ostream ods(&odsbuf);

	} // namespace debug_yes

	namespace debug_no {

		PseudoNullStream ods;

		class nullstreambuf : public std::streambuf {};

		nullstreambuf nullbuf;
		std::ostream ns(&nullbuf);

	} // namespace debug_no

} // namespace yak
