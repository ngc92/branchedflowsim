#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

bool compare_files_binary(string f1, string f2, bool verbose = true)
{
	if(verbose)
		std::cout << "comparing files " << f1 << " " << f2 << "\n";

	fstream s1( f1, fstream::in | fstream::binary );
	fstream s2( f2, fstream::in | fstream::binary );

	if( !s1.is_open() || !s2.is_open())
	{
		std::cerr << "error opening file to compare " << f1 << " and " << f2 << "\n";
		return false;
	}

	while( !(s1.eof() || s2.eof()) )
	{
		char c1 = s1.get();
		char c2 = s2.get();
		if(c1 != c2)
		{
			std::cout << "file differ at position "<< s1.tellg() << ": " << (int)c1 << " vs " << (int)c2 << " \n";
			return false;
		}
	}

	if( !(s1.eof() && s2.eof()) )
	{
		std::cout << "file length is different\n";
		return false;
	}

	if(verbose)
		std::cout << "files are identical\n";
	return true;
}
