#include "AsynchInput.h"
#include <Windows.h>

static HANDLE s_inHandle = nullptr;
static HANDLE s_outHandle = nullptr;

static std::vector<std::string> s_keywords;
static decltype(s_keywords)::iterator s_lastAutoComplete = s_keywords.end();
static std::string s_lastCompleteWord;

static std::vector<std::string> s_lastWords;
static size_t currentWordIndex = 0;

static std::mutex s_muKeywords;

std::string AsynchInput::readConsole()
{
	std::string input;
	INPUT_RECORD ir;
	
	currentWordIndex = s_lastWords.size();

	bool pressedTab = false;
	while(true)
	{		
		DWORD cNumRead;

		if(!ReadConsoleInputA(
		s_inHandle,
		&ir,
		1,
		&cNumRead
		)) continue;
		
		if(!cNumRead) continue;
		if(ir.EventType != KEY_EVENT) continue;

		auto& e = ir.Event.KeyEvent;
		if (!e.bKeyDown) continue;

		// reset and remember tab state
		const auto prevPressedTab = pressedTab;
		pressedTab = false;

		switch (e.wVirtualKeyCode)
		{
		case VK_RETURN:
			// accept input
			std::cout << '\n';

			s_lastWords.push_back(input);
			return input;
		case VK_TAB:
			// auto complete
			// find best match
			{
				pressedTab = true;
				std::lock_guard<std::mutex> g(s_muKeywords);
				// find best match
				// TODO extract the current word from input
				auto w = input;
				const auto lastSpace = w.find_last_of(' ');
				if(lastSpace != std::string::npos)
				{
					w = w.substr(lastSpace + 1);
				}

				// try the word from last time again
				if (prevPressedTab)
					w = s_lastCompleteWord;

				auto begin = s_keywords.begin();
				const bool firstSearch = !prevPressedTab;
				size_t prevWordLenghtDiff = 0;

				s_lastCompleteWord = w;
				if(!firstSearch)
				{
					// start from last point
					begin = s_lastAutoComplete;
					if (begin != s_keywords.end())
					{
						prevWordLenghtDiff = begin->length() - w.length();
						++begin;
					}
				}

				// try to find the first match between two words
				const auto matcher = [&w](const std::string& word)
				{
					if (w.length() >= word.length()) return false;
					return std::equal(w.begin(), w.end(), word.begin());
				};

				s_lastAutoComplete = std::find_if(begin, s_keywords.end(), matcher);

				if(s_lastAutoComplete == s_keywords.end() && !firstSearch)
				{
					// just start again from the beginning
					s_lastAutoComplete = std::find_if(s_keywords.begin(), s_keywords.end(), matcher);
				}

				if (s_lastAutoComplete == s_keywords.end())
					break; // no matches found
					
				if(!firstSearch)
				{
					// erase previous characters
					for(auto i = 0; i < prevWordLenghtDiff; ++i)
					{
						std::cout << "\b \b";
						input.pop_back();
					}
				}
				// display this match
				const auto sub = s_lastAutoComplete->substr(w.length());
				input.append(sub);
				std::cout << sub;
			}
			break;
		case VK_UP:
			// display previous word
		{
			if (currentWordIndex == 0) continue;
			--currentWordIndex;

			// erase current word
			for (size_t i = 0; i < input.length(); ++i)
				std::cout << "\b \b";

			input = s_lastWords[currentWordIndex];
			std::cout << input;
		}
			break;
		case VK_DOWN:
			// display next word
			if (currentWordIndex + 1 >= s_lastWords.size()) continue;
			++currentWordIndex;

			// erase current word
			for (size_t i = 0; i < input.length(); ++i)
				std::cout << "\b \b";

			input = s_lastWords[currentWordIndex];
			std::cout << input;
			break;
		case VK_BACK:
			// delete last input
			if(input.length())
			{
				std::cout << "\b \b";
				input.pop_back();
			}
			break;
		default:
			// dont print the null character
			if (e.uChar.UnicodeChar)
			{
				std::cout << char(e.uChar.AsciiChar);
				input.push_back(char(e.uChar.AsciiChar));
			}
			break;
		}
	}


	return "";
	/*std::string a;
	std::getline(std::cin, a, '\n');
	return a;	*/
}

AsynchInput::AsynchInput()
{
	start();
	
	if(!s_outHandle)
		s_outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	
	if(!s_inHandle)
		s_inHandle = GetStdHandle(STD_INPUT_HANDLE);
}

void AsynchInput::setKeywords(std::vector<std::string> keywords)
{
	std::lock_guard<std::mutex> g(s_muKeywords);
	s_keywords = keywords;
	s_lastAutoComplete = s_keywords.end();
}
