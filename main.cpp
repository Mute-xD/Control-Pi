#include "stdafx.h"
#include "Network.h"



int main()
{
    Status status;
    NetThreadData netData{62628, &status};

    pthread_t threads[2];
    int rc;


    rc = pthread_create(&threads[0], nullptr, networkThread, (void *) &netData);
    if (rc)
    {
        std::cout << "Threading Failed" << std::endl;
        exit(-1);
    }
    std::cout << "Done!" << std::endl;
    pthread_join(threads[0], nullptr);
}
