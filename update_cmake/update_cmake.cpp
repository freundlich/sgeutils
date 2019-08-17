#include <fcppt/noncopyable.hpp>
#include <fcppt/config/external_begin.hpp>
#include <filesystem>
#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <regex>
#include <string>
#include <vector>
#include <cstdlib>
#include <fcppt/config/external_end.hpp>


namespace
{

typedef std::vector<
	std::string
> file_vector;

template<
	typename Iterator
>
void
add_files(
	Iterator _iterator,
	std::regex const &_regex,
	file_vector &_files,
	std::filesystem::path const &_out_file
)
{
	for(
		;
		_iterator != Iterator();
		++_iterator
	)
	{
		if(
			std::filesystem::equivalent(
				_out_file,
				*_iterator
			)
		)
			continue;

		if(
			!std::filesystem::is_regular_file(
				*_iterator
			)
		)
			continue;

		if(
			!std::regex_match(
				_iterator->path().filename().string(),
				_regex
			)
		)
			continue;

		_files.push_back(
			_iterator->path().generic_string()
		);
	}
}

class out_remover
{
	FCPPT_NONCOPYABLE(
		out_remover
	);
public:
	explicit
	out_remover(
		std::filesystem::path const &_file
	)
	:
		file_(
			_file
		),
		remove_(
			true
		)
	{
	}

	~out_remover()
	{
		if(
			remove_
		)
			std::filesystem::remove(
				file_
			);
	}

	void
	commit()
	{
		remove_ = false;
	}
private:
	std::filesystem::path const file_;

	bool remove_;
};

}

int
main(
	int argc,
	char **argv
)
try
{
	if(
		argc <= 3
	)
	{
		std::cerr
			<< "Usage: "
			<< argv[0]
			<< " <CMakeFile> <VAR_NAME> <path1> [path2] ...\n"
			<< "In front of every path the additional options -r, -n and -e are accepted.\n"
			<< "-r will search recursively, while -n will not.\n"
			<< "-e will change the regex the filenames have to match.\n"
			<< "The default is -r -e \".*\\..pp\"\n";

		return EXIT_FAILURE;
	}

	std::filesystem::path const cmake_file(
		argv[1]
	);

	std::ifstream ifs(
		cmake_file
	);

	if(
		!ifs.is_open()
	)
	{
		std::cerr
			<< cmake_file
			<< " does not exist.\n";

		return EXIT_FAILURE;
	}

	std::filesystem::path const out_file(
		cmake_file.string()
		+
		".new"
	);

	out_remover remove_ofs(
		out_file
	);

	std::ofstream ofs(
		out_file
	);

	if(
		!ofs.is_open()
	)
	{
		remove_ofs.commit();

		std::cerr
			<< "Cannot open "
			<< out_file
			<< '\n';

		return EXIT_FAILURE;
	}

	file_vector files;

	std::string mode(
		"r"
	);

	std::regex fileregex(
		".*\\..pp"
	);

	for(
		int arg = 3;
		arg < argc;
		++arg
	)
	{
		std::string const arg_string(
			argv[arg]
		);

		if(
			!arg_string.empty()
			&&
			arg_string[0] == '-'
		)
		{
			if(
				arg_string.size() < 2
			)
			{
				std::cerr
					<< "- must be followed by an option\n";

				return EXIT_FAILURE;
			}

			if(
				arg_string[1]
				== 'n'
				||
				arg_string[1]
				== 'r'
			)
				mode =
					arg_string.substr(
						1u
					);
			else if(
				arg_string[1]
				== 'e'
			)
			{
				++arg;

				if(
					arg == argc
				)
				{
					std::cerr
						<< "-e must be followed by a regex!\n";

					return EXIT_FAILURE;
				}

				fileregex =
					argv[arg];
			}

			continue;
		}

		if(
			mode == "r"
		)
			::add_files(
				std::filesystem::recursive_directory_iterator(
					arg_string
				),
				fileregex,
				files,
				out_file
			);
		else if(
			mode == "n"
		)
			::add_files(
				std::filesystem::directory_iterator(
					arg_string
				),
				fileregex,
				files,
				out_file
			);
		else
		{
			std::cerr
				<< "Invalid mode "
				<< mode
				<< '\n';

			return EXIT_FAILURE;
		}
	}

	std::sort(
		files.begin(),
		files.end()
	);

	std::string const search_line(
		std::string(
			"\t"
		)
		+ argv[2]
	);

	std::string line;

	while(
		std::getline(
			ifs,
			line
		)
		&&
		line
		!= search_line
	)
		ofs
			<< line
			<< '\n';

	if(
		line != search_line
	)
	{
		std::cerr
			<< search_line
			<< " not found!\n";

		return EXIT_FAILURE;
	}

	ofs
		<< line
		<< '\n';

	for(
		file_vector::const_iterator it(
			files.begin()
		);
		it != files.end();
		++it
	)
		ofs
			<< '\t'
			<< *it
			<< '\n';

	std::string const search_end(
		")"
	);

	while(
		std::getline(
			ifs,
			line
		)
		&&
		line
		!= search_end
	) ;

	if(
		line != search_end
	)
	{
		std::cerr
			<< search_end
			<< " not found!\n";

		return EXIT_FAILURE;
	}

	ofs
		<< line
		<< '\n';

	while(
		std::getline(
			ifs,
			line
		)
	)
		ofs
			<< line
			<< '\n';

	ofs.close();

	ifs.close();

	remove_ofs.commit();

	std::filesystem::rename(
		out_file,
		cmake_file
	);

	return EXIT_SUCCESS;
}
catch(
	std::exception const &_error
)
{
	std::cerr
		<< _error.what()
		<< '\n';

	return EXIT_FAILURE;
}
