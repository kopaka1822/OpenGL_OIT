#include "AsynchInput.h"
#include <Windows.h>

static HANDLE s_inHandle = nullptr;
static HANDLE s_outHandle = nullptr;

static std::vector<std::string> s_keywords;
static decltype(s_keywords)::iterator s_lastAutoComplete = s_keywords.end();
static std::string s_lastCompleteWord;

static std::vector<std::string> s_lastWords;
static size_t currentWordIndex = 0;

// the recorded string
static std::string s_input;
static bool s_pressedTab = false;

std::string AsynchInput::get()
{
	INPUT_RECORD ir;
	DWORD cNumRead;

	currentWordIndex = s_lastWords.size();

	while (true)
	{
		if (!PeekConsoleInputA(
			s_inHandle,
			&ir,
			1,
			&cNumRead)) break;
		
		// nothing available to read
		if (!cNumRead) break;

		// process the data
		if(!ReadConsoleInputA(
			s_inHandle,
			&ir,
			1,
			&cNumRead)) break;
		
		if(!cNumRead) break;
		if(ir.EventType != KEY_EVENT) continue;

		auto& e = ir.Event.KeyEvent;
		if (!e.bKeyDown) continue;

		// reset and remember tab state
		const auto prevPressedTab = s_pressedTab;
		s_pressedTab = false;

		switch (e.wVirtualKeyCode)
		{
		case VK_RETURN:
			// accept s_input
			std::cout << '\n';

			s_lastWords.push_back(s_input);
			return s_input;
		case VK_TAB:
			// auto complete
			// find best match
			{
				s_pressedTab = true;
				// find best match
				// TODO extract the current word from s_input
				auto w = s_input;
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
						s_input.pop_back();
					}
				}
				// display this match
				const auto sub = s_lastAutoComplete->substr(w.length());
				s_input.append(sub);
				std::cout << sub;
			}
			break;
		case VK_UP:
			// display previous word
		{
			if (currentWordIndex == 0) continue;
			--currentWordIndex;

			// erase current word
			for (size_t i = 0; i < s_input.length(); ++i)
				std::cout << "\b \b";

			s_input = s_lastWords[currentWordIndex];
			std::cout << s_input;
		}
			break;
		case VK_DOWN:
			// display next word
			if (currentWordIndex + 1 >= s_lastWords.size()) continue;
			++currentWordIndex;

			// erase current word
			for (size_t i = 0; i < s_input.length(); ++i)
				std::cout << "\b \b";

			s_input = s_lastWords[currentWordIndex];
			std::cout << s_input;
			break;
		case VK_BACK:
			// delete last s_input
			if(s_input.length())
			{
				std::cout << "\b \b";
				s_input.pop_back();
			}
			break;
		default:
			// dont print the null character
			if (e.uChar.UnicodeChar)
			{
				std::cout << char(e.uChar.AsciiChar);
				s_input.push_back(char(e.uChar.AsciiChar));
			}
			else
			{
				// some random key was pressed. just keave the tab state
				s_pressedTab = prevPressedTab;
			}
			break;
		}
	}

	return "";
}

void AsynchInput::init()
{
	s_outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	s_inHandle = GetStdHandle(STD_INPUT_HANDLE);
}

void AsynchInput::setKeywords(std::vector<std::string> keywords)
{
	s_keywords = keywords;
	s_lastAutoComplete = s_keywords.end();
}
