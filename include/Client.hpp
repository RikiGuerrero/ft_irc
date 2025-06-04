#ifndef CLIENT_HPP
    #define CLIENT_HPP

#include <string>
#include <sstream>
#include <unistd.h>

class Client
{
    private:
        int _fd;
        bool _authenticated;
        std::string _recvBuffer;
        std::string _sendBuffer;
        std::string _nickname;
        std::string _username;
        std::string _hostname;
        std::string _realname;
        bool _hasPass;
        bool _hasNick;
        bool _hasUser;
    
    public:
        Client(int fd, const std::string &hostname);
        ~Client();
        
        std::string &getRecvBuffer();
        void appendToBuffer(const std::string &data);

        int getFd() const;
        const std::string &getNickname() const;
        const std::string &getUsername() const;
        const std::string &getHostname() const;
        const std::string &getSendBuffer() const;

        bool isAuthenticated() const;
        bool hasAllInfo() const;
        bool isRegistered() const;
        bool isPassSet() const;

        void setNickname(const std::string &nickname);
        void setUsername(const std::string &username);
        void setRealname(const std::string &realname);
        void setPass(bool ok);
        void appendToSendBuffer(const std::string &data);
        void clearSendBuffer();
        void eraseSendBuffer(size_t count);

        void tryAuthenticate();

        std::string getPrefix() const;
};

#endif