// Opening-scoped introduction authorization. No engine dependencies, so the
// protocol/lifecycle rules can be checked independently of a Windows client build.
#ifndef INCLUDED_SwgCuiHolocronIntroduction_H
#define INCLUDED_SwgCuiHolocronIntroduction_H

#include <sstream>
#include <string>

namespace SwgCuiHolocronIntroduction
{
    inline unsigned int nextOpeningId(unsigned int previous)
    {
        return previous >= 2147483647u ? 1u : previous + 1u;
    }

    inline std::string activationResponse(unsigned int openingId)
    {
        std::ostringstream result;
        result << "holocronActivated:" << openingId;
        return result.str();
    }

    inline bool parseAuthorization(std::string const & request, unsigned int & openingId)
    {
        std::string const prefix("holocronIntro:");
        if (request.compare(0, prefix.size(), prefix) != 0)
            return false;
        std::string::size_type const digits = request.size() - prefix.size();
        if (digits < 1 || digits > 10 || request[prefix.size()] == '0')
            return false;
        unsigned int result = 0;
        for (std::string::size_type i = prefix.size(); i < request.size(); ++i)
        {
            char const digit = request[i];
            if (digit < '0' || digit > '9')
                return false;
            unsigned int const value = static_cast<unsigned int>(digit - '0');
            if (result > (2147483647u - value) / 10u)
                return false;
            result = result * 10u + value;
        }
        openingId = result;
        return true;
    }

    class Opening
    {
    public:
        Opening() : m_id(0), m_authorized(false) {}

        void begin(unsigned int id)
        {
            m_id = id;
            m_authorized = false;
        }

        void end()
        {
            m_id = 0;
            m_authorized = false;
        }

        bool authorize(std::string const & request)
        {
            unsigned int requestedId = 0;
            if (m_id == 0 || m_authorized || !parseAuthorization(request, requestedId) || requestedId != m_id)
                return false;
            m_authorized = true;
            return true;
        }

    private:
        unsigned int m_id;
        bool m_authorized;
    };
}

#endif
