/*
This file is part of OpenModelica.

Copyright (c) 1998-2005, Linköpings universitet, Department of
Computer and Information Science, PELAB

All rights reserved.

(The new BSD license, see also
http://www.opensource.org/licenses/bsd-license.php)


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

* Neither the name of Linköpings universitet nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <list>
#include <string>
#include <sstream>
#include <iostream>

#include "ErrorMessage.hpp"

  /* Implementation of ErrorMessage class. */


  ErrorMessage::ErrorMessage(long errorID,
			     std::string type,
			     std::string severity,
			     std::string message,
			     std::list<std::string>& tokens) 
    : errorID_(errorID),
      messageType_(type),
      severity_(severity),
      message_(message),
      tokens_(tokens)
							      
{
  lineNo_ = 0; 
  columnNo_ = 0;
  filename_ = std::string("");
}

ErrorMessage::ErrorMessage(long errorID,
			   std::string type,
			   std::string severity,
			   std::string message,
			   std::list<std::string> &tokens,
			   long lineNo,
			   long columnNo,
			   std::string filename) 
    :
    errorID_(errorID),
    messageType_(type),
    severity_(severity),
    lineNo_(lineNo),
    columnNo_(columnNo),
    filename_(filename),
    message_(message),
    tokens_(tokens)
{
}


std::string ErrorMessage::getMessage() 
{
  std::string fullMessage = message_;
  std::list<std::string>::iterator tok;
  std::string::size_type str_pos;
  for (tok=tokens_.begin(); tok != tokens_.end(); tok++) {
    str_pos=fullMessage.find("%s");
    if (str_pos < fullMessage.size()) {
      fullMessage.replace(str_pos,2,*tok);
    }
    else {
      std::cerr << "Internal error in error handling, no %s left to replace "<< *tok << " with." << std::endl;
    }
	
  }
  std::stringstream str;
  str << "["<< filename_ << ":" << lineNo_ << ":" << columnNo_ << "]: ";
  std::string s1 = str.str();
  if (filename_ == "" && lineNo_ == 0 && columnNo_ == 0) {
    return fullMessage;
  } else {
    return s1+fullMessage;
  }
}

std::string ErrorMessage::getFullMessage()
{
  std::string message_text= getMessage();

  std::stringstream strbuf;

  strbuf << "{\"" << message_text << "\", \"" <<
    messageType_ << "\", \"" <<
    severity_ << "\", \"" <<
    errorID_ << "\"}" << std::ends;
  return strbuf.str();
}
