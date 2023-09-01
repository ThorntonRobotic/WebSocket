#include "logger.hpp"
#include <chrono>
#include <functional>
#include <thread>

#ifdef _WIN32
#include <winsock.h>
#include <Windows.h>
#include<conio.h>
#else
#include <unistd.h>
#endif

#include <srt.h>
#include <access_control.h>
#include <utilities.h>


// Test program for endpoint server using functions as the callback agent

Logger log_("", "");


/*
Access Control is sent from client to server and a string is passed in the accept callback
There is a defacto format where the string begins with "#!::" and contains name=value,...
u: User Name, or authorization name, - that is expected to control which password should be used for the connection. The application should interpret it to distinguish which user should be used by the listener party to set up the password.
r: Resource Name identifies the name of the resource and facilitates selection should the listener party be able to serve multiple resources.
h: Host Name identifies the hostname of the resource. For example, to request a stream with the URI somehost.com/videos/querry.php?vid=366 the hostname field should have somehost.com, and the resource name can have videos/querry.php?vid=366 or simply 366. Note that this is still a key to be specified explicitly. Support tools that apply simplifications and URI extraction are expected to insert only the host portion of the URI here.
s: Session ID is a temporary resource identifier negotiated with the server, used just for verification. This is a one-shot identifier, invalidated after the first use. The expected usage is when details for the resource and authorization are negotiated over a separate connection first, and then the session ID is used here alone.
t: Type specifies the purpose of the connection. Several standard types are defined, but users may extend the use:
    stream (default, if not specified): for exchanging the user-specified payload for an application-defined purpose
    file: for transmitting a file, where r is the filename
    auth: for exchanging sensible data. The r value states its purpose. No specific possible values for that are known so far (FUTURE USE]
m: Mode expected for this connection:
    request (default): the caller wants to receive the stream
    publish: the caller wants to send the stream data
    bidirectional: bidirectional data exchange is expected
*/

class AccessControl
{
public:
    AccessControl(){
        // initialization code here
    }

    ~AccessControl(){
        // cleanup any pending stuff, but no exceptions allowed
    }

protected:
    // In order to use a class member callback we have to set up a static function and cast the callback to
    // use the appropriate instance. The srt library does not seem to have a friendlier way like ::bind.
    static int acceptCallback(void* opaq, SRTSOCKET ns, int hsversion, const struct sockaddr* peeraddr, const char* streamid){
        return reinterpret_cast<AccessControl *>(opaq)->acceptHandler(ns, hsversion, peeraddr, streamid);
    }

    int acceptHandler(SRTSOCKET ns, int hsversion, const struct sockaddr* peeraddr, const char* streamid)
    {
        using namespace std;

        // opaq is used to pass some further chained callbacks

        // To reject a connection attempt, return -1.

        static const map<string, string> passwd {
            {"admin", "thelocalmanager"},
            {"user", "verylongpassword"}
        };

        // Try the "standard interpretation" with username at key u
        string username;

        static const char stdhdr [] = "#!::";
        uint32_t* pattern = (uint32_t*)stdhdr;
        bool found = -1;

        // Extract a username from the StreamID:
        if (strlen(streamid) > 4 && *(uint32_t*)streamid == *pattern){
            vector<string> items;
            Split(streamid+4, ',', back_inserter(items));
            for (auto& i: items){
                vector<string> kv;
                Split(i, '=', back_inserter(kv));
                if (kv.size() == 2 && kv[0] == "u"){
                    username = kv[1];
                    found = true;
                }
            }

            if (!found){
                cerr << "TEST: USER NOT FOUND, returning false.\n";
                return -1;
            }
        }
        else{
            // By default the whole streamid is username
            username = streamid;
        }

        // When the username of the client is known, the passphrase can be set
        // on the socket being accepted (SRTSOCKET ns).
        // The remaining part of the SRT handshaking process will check the
        // passphrase of the client and accept or reject the connection.

        // When not found, it will throw an exception
        cerr << "TEST: Accessing user '" << username << "', might throw if not found\n";
        string exp_pw = passwd.at(username);

        cerr << "TEST: Setting password '" << exp_pw << "' as per user '" << username << "'\n";
        //srt_setsockflag(ns, SRTO_PASSPHRASE, exp_pw.c_str(), exp_pw.size());
        return 0;
    }
public:

    SRTSOCKET server_sock, client_sock;
    std::thread accept_thread;
    sockaddr_in sa;
    sockaddr* psa;

    void setup()
    {
        // Create server on 127.0.0.1:5555

        server_sock = srt_create_socket();
        if(server_sock <= 0){    // socket_id should be > 0
            std::cout << "Unable to create srt server socket" << std::endl;
            return;
        }

        sockaddr_in bind_sa;
        memset(&bind_sa, 0, sizeof bind_sa);
        bind_sa.sin_family = AF_INET;
        if(inet_pton(AF_INET, "127.0.0.1", &bind_sa.sin_addr) != 1){
          std::cout << "Unable to set up srt server inet_pton" << std::endl;
            return;  
        }
        bind_sa.sin_port = htons(5555);

        if(srt_bind(server_sock, (sockaddr*)&bind_sa, sizeof bind_sa) == -1){
             std::cout << "Unable to bind srt socket" << std::endl;
            return;  
        }
        
        if(srt_listen(server_sock, 5) == -1){
            std::cout << "Unable to listen on srt socket" << std::endl;
            return;  
        }

        (void)srt_listen_callback(server_sock, &AccessControl::acceptCallback, this);

        accept_thread = std::thread([this] { this->AcceptLoop(); });

        // Prepare client socket

        client_sock = srt_create_socket();
        memset(&sa, 0, sizeof sa);
        sa.sin_family = AF_INET;
        sa.sin_port = htons(5555);
        if(inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr) != 1){
            std::cout << "Unable to set up srt client inet_pton" << std::endl;
            return;  
        }
        psa = (sockaddr*)&sa;

        if(client_sock <= 0){    // socket_id should be > 0
            std::cout << "Unable to create srt server socket" << std::endl;
            return;
        }

        auto awhile = std::chrono::milliseconds(20);
        std::this_thread::sleep_for(awhile);
    }

    void connectClient(){
        std::string username_spec = "#!::u=admin";
        std::string password = "thelocalmanager";

        if (srt_setsockflag(client_sock, SRTO_STREAMID, username_spec.c_str(), username_spec.size()) == -1){
        }

        if(srt_connect(client_sock, psa, sizeof sa) ==  SRT_ERROR){
        }

        if(srt_getrejectreason(client_sock) != SRT_REJ_UNKNOWN){
        }
    }

    void AcceptLoop()
    {
        // Setup EID in order to pick up either readiness or error.
        // THis is only to make a formal response side, nothing here is to be tested.

        int eid = srt_epoll_create();

        // Subscribe to R | E

        int re = SRT_EPOLL_IN | SRT_EPOLL_ERR;
        srt_epoll_add_usock(eid, server_sock, &re);

        SRT_EPOLL_EVENT results[2];

        for (;;)
        {
            auto state = srt_getsockstate(server_sock);
            if (int(state) > int(SRTS_CONNECTED))
            {
                std::cout << "[T] Listener socket closed, exitting\n";
                break;
            }

            std::cout << "[T] Waiting for epoll to accept\n";
            int res = srt_epoll_uwait(eid, results, 2, 1000);
            if (res == 1)
            {
                if (results[0].events == SRT_EPOLL_IN)
                {
                    SRTSOCKET acp = srt_accept(server_sock, NULL, NULL);
                    if (acp == SRT_INVALID_SOCK)
                    {
                        std::cout << "[T] Accept failed, so exitting\n";
                        break;
                    }
                    srt_close(acp);
                    continue;
                }

                // Then it can only be SRT_EPOLL_ERR, which
                // can be done by having the socket closed
                break;
            }

            if (res == 0) // probably timeout, just repeat
            {
                std::cout << "[T] (NOTE: epoll timeout, still waiting)\n";
                continue;
            }
        }

        srt_epoll_release(eid);
    }

    void teardown()
    {
        std::cout << "TearDown: closing all sockets\n";
        // Close the socket
        srt_close(client_sock);//, SRT_SUCCESS);
        srt_close(server_sock);//, SRT_SUCCESS);

        // After that, the thread should exit
        std::cout << "teardown: joining accept thread\n";
        accept_thread.join();
        std::cout << "teardown: SRT exit\n";
    }

};



int main() {

    AccessControl srtTest;
    srtTest.setup();
    srtTest.connectClient();
    return 0;
}