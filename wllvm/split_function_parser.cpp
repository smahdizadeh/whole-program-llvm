//must install libclang for this parser:sudo apt-get install -y libclang-dev
//must compile: clang++ --std=c++11 function_size_parser.cpp -lclang
#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#ifdef __unix__
#include <limits.h>
#include <stdlib.h>
#endif
#include<stdio.h> 
#include<string.h> 
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <fstream>
#include <limits>
#include <sys/stat.h>
#include <cstdlib>

std::fstream openedFile;
std::string OutputFolderPath;
std::fstream& GotoLine(std::fstream& file, unsigned int num){
	file.seekg(std::ios::beg);
	for(int i=0; i < num - 1; ++i){
		file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
	}
	return file;
}

bool is_function_definition(std::fstream& file, int startLine, int endLine, int startColumn, int endColumn){
	GotoLine(file, startLine);
	std::string line;
	for(int i = 0; i < endLine - startLine + 1; i++) {
		std::getline(openedFile, line);
		if (endLine == startLine) {
			for (int j = startColumn - 1; j <= endColumn - 1; j++) {
				if(line[j]== '{'){
					return true;
				}
			}
			return false;
		}
		else if (i == 0) {
			for (int j = startColumn - 1; j < line.size(); j++) {
				if(line[j]== '{'){
                                        return true;
                                }
			}
		}
		else if (i == endLine - startLine) {
			for (int j = 0; j <= endColumn - 1; j++) {
				if(line[j]== '{'){
                                        return true;
                                }

			}
		}
		else {
                        for (int j = 0; j < line.size(); j++) {
                                if(line[j]== '{'){
                                        return true;
                                }
                        
                        }

		}
	}
	return false;
}

std::string getCursorSpelling( CXCursor cursor )
{
	CXString cursorSpelling = clang_getCursorSpelling( cursor );
	std::string result      = clang_getCString( cursorSpelling );

	clang_disposeString( cursorSpelling );
	return result;
}

/* Auxiliary function for resolving a (relative) path into an absolute path */
std::string resolvePath( const char* path )
{
	std::string resolvedPath;

#ifdef __unix__
	char* resolvedPathRaw = new char[ PATH_MAX ];
	char* result          = realpath( path, resolvedPathRaw );

	if( result )
		resolvedPath = resolvedPathRaw;

	delete[] resolvedPathRaw;
#else
	resolvedPath = path;
#endif

	return resolvedPath;
}


inline bool fileExists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}


CXChildVisitResult functionVisitor( CXCursor cursor, CXCursor /* parent */, CXClientData /* clientData */ )
{
	if( clang_Location_isFromMainFile( clang_getCursorLocation( cursor ) ) == 0 )
		return CXChildVisit_Continue;

	CXCursorKind kind = clang_getCursorKind( cursor );
	auto name         = getCursorSpelling( cursor );

	if( kind == CXCursorKind::CXCursor_FunctionDecl 
			|| kind == CXCursorKind::CXCursor_CXXMethod 
			|| kind == CXCursorKind::CXCursor_FunctionTemplate 
			|| kind == CXCursorKind::CXCursor_ObjCInstanceMethodDecl
			|| kind == CXCursorKind::CXCursor_ObjCClassMethodDecl 
			|| kind == CXCursorKind::CXCursor_Constructor
			|| kind == CXCursorKind::CXCursor_Destructor
			|| kind == CXCursorKind::CXCursor_ConversionFunction)
	{
		CXSourceRange extent           = clang_getCursorExtent( cursor );
		CXSourceLocation startLocation = clang_getRangeStart( extent );
		CXSourceLocation endLocation   = clang_getRangeEnd( extent );

		unsigned int startLine = 0, startColumn = 0;
		unsigned int endLine   = 0, endColumn   = 0;

		clang_getSpellingLocation( startLocation, nullptr, &startLine, &startColumn, nullptr );
		clang_getSpellingLocation( endLocation,   nullptr, &endLine, &endColumn, nullptr );

		bool is_definition = is_function_definition(openedFile, startLine, endLine, startColumn, endColumn);
		if(!is_definition){
			return CXChildVisit_Recurse;
		}
		std::string mangled_name = (char*)clang_Cursor_getMangling (cursor).data;
//		std::cout  << mangled_name << "\n";
//		std::cout << "  " << name << ": " << endLine - startLine << " " << startColumn << " " << endColumn << "\n";

		std::string OutputFilename = OutputFolderPath + "/func_" + mangled_name + ".cpp";
		if (fileExists(OutputFilename)) {
			std::cout << "Duplicate function name: "<<mangled_name<<" terminating function spliting...\n";
			assert(false);
		}
		std::ofstream fouts(OutputFilename.c_str());
//		std::cout  << OutputFilename << "\n";
		if (!fouts.is_open()) {
			fouts << "Bad path, cant open output file ...\n";
			assert(false);
		} 

		//if we are here, this is a function definition
		GotoLine(openedFile, startLine);
		std::string line;
		for(int i = 0; i < endLine - startLine + 1; i++) {
			std::getline(openedFile, line);
			if (endLine == startLine) {
				for (int j = startColumn - 1; j <= endColumn - 1; j++) {
					fouts << line[j];
				}
				fouts << "\n";
				break;
			}
			else if (i == 0) {
				for (int j = startColumn - 1; j < line.size(); j++) {
					fouts << line[j];
				}
				fouts << "\n";
			}
			else if (i == endLine - startLine) {
				for (int j = 0; j <= endColumn - 1; j++) {
					fouts << line[j];
				}
				fouts << "\n";
			}
			else {
				fouts << line << "\n";
			}
		}
		fouts.close();
	}

	return CXChildVisit_Recurse;
}

// int getFileIndex(int argc, char** argv) {
// 	for (int i = 1; i < argc; i++) {
// 		if (argv[i][0] == '-') {
// 			continue;
// 		}
// 	}
// }

int main( int argc, char** argv )
{
	if( argc < 2 )
		return -1;

	int file_index = argc-1;
	auto resolvedPath = resolvePath(argv[file_index]);
	openedFile.open(resolvedPath.c_str());
	std::cerr << "Parsing " << resolvedPath << "...\n";

	if(std::getenv("SPLIT_FUNCTION_PARSER_OUTPUT_FOLDER_PATH") != NULL){
		OutputFolderPath = std::getenv("SPLIT_FUNCTION_PARSER_OUTPUT_FOLDER_PATH");
	}
	else{
		std::cerr<<"SPLIT_FUNCTION_PARSER_OUTPUT_FOLDER_PATH is not defined...\n";
		assert(false);
	}
	//  CXCompilationDatabase_Error compilationDatabaseError;
	//  CXCompilationDatabase compilationDatabase = clang_CompilationDatabase_fromDirectory( ".", &compilationDatabaseError );
	//  CXCompileCommands compileCommands         = clang_CompilationDatabase_getCompileCommands( compilationDatabase, resolvedPath.c_str() );
	//  unsigned int numCompileCommands           = clang_CompileCommands_getSize( compileCommands );
	unsigned int numCompileCommands = 0;

	//std::cerr << "Obtained " << numCompileCommands << " compile commands\n";

	CXIndex index = clang_createIndex( 0, 1 );
	CXTranslationUnit translationUnit;

	if( numCompileCommands == 0 )
	{
		char** arguments = new char* [argc + 1];
		int cnt = 0;
		for (int i = 1; i < argc; i++) {
			if (i == file_index)
				continue;
			arguments[cnt] = strdup(argv[i]);
			cnt++;
		}
		arguments[cnt++] = strdup("-I/usr/include");
		arguments[cnt++] = strdup("-I/usr/local/lib/clang/3.9.0/include");
		arguments[cnt++] = strdup("-I/usr/local/include");

		for (int i = 0; i< cnt; i++){
			std::cout<<arguments[i]<<"\n";
		}
//		constexpr const char* defaultArguments[] = {
//			"-std=c++11",
//			"-I/usr/include",
//			"-I/usr/local/lib/clang/3.9.0/include",
//			"-I/usr/local/include"
//		};
	//	std::cerr<<"start clang parsing....\n";

		CXErrorCode err_code = clang_parseTranslationUnit2( index,
				resolvedPath.c_str(),
				arguments,
				cnt,
				0,
				0,
				CXTranslationUnit_None, &translationUnit );
		
	//	std::cerr<<"error code= "<<err_code << "\n";
		if(err_code != 0){
			std::cerr<<"clang parser was unsuccessful, exiting...\n";
			return -1;
		}

	}
	/*  else
	    {
	    CXCompileCommand compileCommand = clang_CompileCommands_getCommand( compileCommands, 0 );
	    unsigned int numArguments       = clang_CompileCommand_getNumArgs( compileCommand );
	    char** arguments                = new char*[ numArguments ];

	    for( unsigned int i = 0; i < numArguments; i++ )
	    {
	    CXString argument       = clang_CompileCommand_getArg( compileCommand, i );
	    std::string strArgument = clang_getCString( argument );
	    arguments[i]            = new char[ strArgument.size() + 1 ];

	    std::fill( arguments[i],
	    arguments[i] + strArgument.size() + 1,
	    0 );

	    std::copy( strArgument.begin(), strArgument.end(),
	    arguments[i] );

	    clang_disposeString( argument );
	    }

	    translationUnit = clang_parseTranslationUnit( index, 0, arguments, numArguments, 0, 0, CXTranslationUnit_None );

	    for( unsigned int i = 0; i < numArguments; i++ )
	    delete[] arguments[i];

	    delete[] arguments;
	    }
	    */
	CXCursor rootCursor = clang_getTranslationUnitCursor( translationUnit );
	clang_visitChildren( rootCursor, functionVisitor, nullptr );
	clang_disposeTranslationUnit( translationUnit );
	clang_disposeIndex( index );
	//  clang_CompileCommands_dispose( compileCommands );
	//  clang_CompilationDatabase_dispose( compilationDatabase );
	return 0;
}
