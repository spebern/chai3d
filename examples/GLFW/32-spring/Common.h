#pragma once

#define DT 0.001

/**
 * \brief trim characters from the left side of the string
 * \param str string to trim
 * \param chars chars to trim
 * \return result of trim operation
 */
inline std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
	str.erase(0, str.find_first_not_of(chars));
	return str;
}

/**
 * \brief trim characters from the right side of the string
 * \param str string to trim
 * \param chars chars to trim
 * \return result of trim operation
 */
inline std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
	str.erase(str.find_last_not_of(chars) + 1);
	return str;
}

/**
 * \brief trim characters from both sides of the string
 * \param str string to trim
 * \param chars chars to trim
 * \return result of trim operation
 */

inline std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
	return ltrim(rtrim(str, chars), chars);
}
