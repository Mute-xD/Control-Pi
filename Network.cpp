//
// Created by Mute on 2022/1/8.
//

#include "Network.h"

Network::Network(int port)
{

    hostent* phst = gethostbyname("local.mutexxd.cn");
    if (!phst)
    {
        perror("DNS ERROR");
    }
    auto* iddr = (in_addr*)phst->h_addr;

    const char* ip = inet_ntoa(*iddr);
    std::cout << "Remote IP: " << ip << std::endl;

    this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == 0)
    {
        perror("StartUp ERROR");
    }
    serverSockAddress.sin_family = AF_INET;
    serverSockAddress.sin_addr.s_addr = inet_addr(ip);
    serverSockAddress.sin_port = htons(port);

}

void Network::Connect()
{
    std::cout << "Connecting to Server..." << std::endl;

    if (connect(this->sock, (struct sockaddr*)&serverSockAddress, sizeof(serverSockAddress)) == -1)
    {
        perror("Connect Failure");
        close(this->sock);
    }
    std::cout << "Server Connected!" << std::endl;
}

void Network::Send(BYTE* data, int dataLen)
{
    int dataLenTotal = dataLen + HEADER_SIZE + 4;
    if (dataLenTotal > BUFFER)
    {
        perror("Package too big");
    }
    BYTE dataBuffer[BUFFER];
    dataBuffer[0] = 0xEF; dataBuffer[1] = 0xEF; dataBuffer[2] = 0xEF; dataBuffer[3] = 0xEF;
    dataBuffer[4] = dataLen / 256;
    dataBuffer[5] = dataLen % 256;
    memcpy(dataBuffer + HEADER_SIZE, data, dataLen);
    dataBuffer[dataLen + HEADER_SIZE] = 0xFE; dataBuffer[dataLen + HEADER_SIZE + 1] = 0xFE; dataBuffer[dataLen + HEADER_SIZE + 2] = 0xFE; dataBuffer[dataLen + HEADER_SIZE + 3] = 0xFE;
    send(sock, (char*)dataBuffer, dataLenTotal, 0);
}

void Network::Recv()
{
    int lastPos = 0;
    int dataLen;
    int dataLenTotal;
    int rcvLen;
    int remainSize;
    BYTE rcvBuffer[BUFFER];
    BYTE dataBuffer[BUFFER];
    BYTE frameData[BUFFER];
    memset(rcvBuffer, 0, sizeof rcvBuffer);
    memset(dataBuffer, 0, sizeof dataBuffer);


    rcvLen = (int)recv(this->sock, (char*)rcvBuffer, BUFFER, 0);
    if (rcvLen > 0)
    {
        memcpy(dataBuffer + lastPos, rcvBuffer, rcvLen);
        lastPos += rcvLen;
        //判断消息缓冲区的数据长度大于消息头
        while (lastPos >= HEADER_SIZE)
        {
            //包头做判断，如果包头错误，收到的数据全部清空
            if (dataBuffer[0] == 0xEF && dataBuffer[1] == 0xEF && dataBuffer[2] == 0xEF && dataBuffer[3] == 0xEF)
            {
                dataLen = (int)dataBuffer[5] + 256 * (int)dataBuffer[4];
                dataLenTotal = dataLen + HEADER_SIZE + 4;
                //判断消息缓冲区的数据长度大于消息体
                if (lastPos >= dataLenTotal)
                {
                    memcpy(frameData, dataBuffer + HEADER_SIZE, dataLen);
                    //处理数据
                    this->RecvProcess(frameData, dataLen);
                    //剩余未处理消息缓冲区数据的长度
                    remainSize = lastPos - dataLenTotal;
                    //将未处理的数据前移
                    if (remainSize > 0)
                    {
                        memcpy(dataBuffer, dataBuffer + dataLenTotal,
                               remainSize);
                        lastPos = remainSize;
                    }
                }

                else
                {
                    break;
                }
            }
            else      //寻找下一个包头
            {
                bool isFind = false;
                int nFindStart = 0;
                for (int k = 1; k < lastPos; k++)
                {
                    if (dataBuffer[k] == 0xEF && dataBuffer[k + 1] == 0xEF && dataBuffer[k + 2] == 0xEF && dataBuffer[k + 3] == 0xEF)
                    {
                        nFindStart = k;
                        isFind = true;
                        break;
                    }
                }
                if (isFind)
                {
                    memcpy(dataBuffer, dataBuffer + nFindStart, lastPos - nFindStart);

                    lastPos = lastPos - nFindStart;
                }
                else
                {
                    memset(dataBuffer, 0, sizeof(dataBuffer));
                    lastPos = 0;
                    break;
                }
            }
            break;

        }
    }
}

void Network::RegistStatus(Status& status1)
{
    {
        this->status = &status1;
    }
}

void Network::RecvProcess(BYTE* frameData, int dataLen)
{
    BYTE path = frameData[0];
    BYTE type = frameData[1];
    BYTE temp;
    bool what;
    switch (path)
    {
        case '0':
            // boardcast
            break;
        case '1':
            switch (type)
            {
                case '0':
                    // s2u command
                    break;
                case '1':
                    // s2u status
                    if (frameData[2] == '6' && frameData[3] == '6' && frameData[4] == '6')
                    {
                        std::cout << "*" << std::endl;
                        status->isServerAlive = true;
                    }
                    break;
                case '2':
                    // s2u message
                    std::cout << "Message From Server" << std::endl;
                    for (auto i = 2; i < dataLen; i++)
                    {
                        std::cout << frameData[i];
                    }
                    std::cout << std::endl;
                    break;
                default:
                    unexpectedPack(frameData, dataLen);
                    break;
            }
        case '6':
            switch (type)
            {
                case '0':
                    // d2u command
                    break;
                case '1':
                    // d2u status

                    break;
                case '2':
                    // u2s message
                    std::cout << "Message From Downer" << std::endl;
                    for (auto i = 2; i < dataLen; i++)
                    {
                        std::cout << frameData[i];
                    }
                    std::cout << std::endl;
                    break;
                default:
                    unexpectedPack(frameData, dataLen);
                    break;
            }
            break;
        default:
            unexpectedPack(frameData, dataLen);
    }
}

void Network::unexpectedPack(BYTE* frameData, int dataLen)
{
    std::cout << "Unexpected Packet Received!" << std::endl;
    std::cout << "Flag is ->" << frameData[0] << std::endl;
    std::cout << "Type is ->" << frameData[1] << std::endl;
    for (auto i = 2; i < dataLen; i++)
    {
        std::cout << frameData[i];
    }
    std::cout << std::endl;
}

void* networkThread(void* data)
{
    auto* threadData = (struct NetThreadData*)data;

    while (!threadData->status->isExit)
    {
        auto* net = new Network(threadData->port);
        net->RegistStatus(*threadData->status);
        net->Connect();
        std::cout << "Start Listening on " << threadData->port << std::endl;
        net->isDisconnected = false;
        DeamonThreadData deamonData{net, threadData->status};
        pthread_t deamon[2];
        int rc;
        rc = pthread_create(&deamon[0], nullptr, deamonSetThread, (void *) &deamonData);
        if (rc)
        {
            std::cout << "Threading Deamon Failed" << std::endl;
            exit(-1);
        }
        rc = pthread_create(&deamon[1], nullptr, deamonCheckThread, (void *) &deamonData);
        if (rc)
        {
            std::cout << "Threading Deamon Failed" << std::endl;
            exit(-1);
        }
        while (!threadData->status->isExit && !net->isDisconnected)
        {
            netJob(*net);
        }
        close(net->sock);
        pthread_join(deamon[0], nullptr);
        pthread_join(deamon[1], nullptr);
        std::cout << "Disconnected! Reset in 5 Sec" << std::endl;
        sleep(5);

    }
    return nullptr;
}

void netJob(Network net)
{
    //BYTE* data = (BYTE*)"Hello World ";
    //net.Send(data, 13);
    net.Recv();
    usleep(100000);
}

void* deamonSetThread(void* data)
{
    auto* threadData = (struct DeamonThreadData*)data;
    while (!threadData->status->isExit && !threadData->net->isDisconnected)
    {
        sleep(PULSE);
        BYTE* pulsePack = (BYTE*)"31666";
        threadData->net->Send(pulsePack, 6);
        std::cout << "Daemon: Pulse Sent" << std::endl;
    }
    std::cout << "Daemon: Set Exit" << std::endl;
    return nullptr;
}

void* deamonCheckThread(void* data)
{
    auto* threadData = (struct DeamonThreadData*)data;
    while (!threadData->status->isExit && !threadData->net->isDisconnected)
    {
        sleep(PULSE * 3);
        if (threadData->status->isServerAlive)
        {
            threadData->status->isServerAlive = false;
        }
        else
        {
            std::cout << "Daemon: Remote No Response" << std::endl;
            threadData->net->isDisconnected = true;
        }
    }
    std::cout << "Daemon: Check Exit" << std::endl;
    return nullptr;
}

