/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 * 
 * DESCRIPTION: Join the distributed system
 * 1.����Ƿ�ΪȺ��������
 * ���ȣ�ͨ���Ƚϵ�ǰ�ڵ�ĵ�ַ�ͼ����ַ��joinaddr������鵱ǰ�ڵ��Ƿ���Ⱥ��������ߣ�Ҳ���ǵ�һ������Ⱥ��Ľڵ㣩��
   ����ǣ��ڵ����Լ��Ѿ���Ⱥ���У�������ִ��һЩȺ���ʼ���Ķ�����

   2.�������������Ϣ
   �����ǰ�ڵ㲻��Ⱥ��������ߣ��ڵ�ṹ��һ����������JOINREQ����Ϣ�����µ�ʵ�ַ�ʽ�У�ʹ��new�ؼ��ֶ�̬����
   һ��MessageHdr���͵Ķ���msg����������Ϣ����ΪJOINREQ��

   ����ǰ�ڵ�ĳ�Ա�б�member_vector��������Ϣ��member_vector�ֶΡ���������Ŀ����Ϊ�˿���ͬ����Ա��Ϣ��
   �Լ���һ���̶�����֤���Ż�����״̬��

   ����ǰ�ڵ�ĵ�ַ������Ϣ��addr�ֶΣ��������շ���֪��������������Ǵ��ĸ��ڵ㷢��ġ�

   3.���ͼ���������Ϣ
   ʹ��emulNet->ENsend����������õļ���������Ϣ���͸������ַ��joinaddr����
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        //size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        //msg = (MessageHdr *) malloc(msgsize * sizeof(char));
        //msg->msgType = JOINREQ;
        // write address behind the type
        //memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        // write heartbeat num behind the type
        //memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
        
        //******a new way********
        // create JOINREQ message: format of data is {struct Address myaddr}
        msg = new MessageHdr();
        msg->msgType = JOINREQ;
        msg->member_vector = memberNode->memberList;

        //add member_vector into the req is to quickly synchronize, with the same data,check and optmize the network
        //address information
        msg->addr = &memberNode->addr;

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, sizeof(MessageHdr));

        //free(msg);
        delete msg;
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 * һ����Ϣ����ص����������ڴ���ֲ�ʽϵͳ�нڵ��յ��Ĳ�ͬ���͵���Ϣ��
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
    MessageHdr* msg = (MessageHdr*)data;
    if (msg->msgType == JOINREQ) {
        //1.add to memberlist
        push_member_list(msg);
        //2.send JOINREP to source node
        Address* toaddr = msg->addr;
        // cout << "JOINREQ : from " << toaddr->getAddress() << " to " << memberNode->addr.getAddress() << endl; 
        send_message(toaddr, JOINREP);   
    }
    else if (msg->msgType == JOINREP) {
        // 1. add to memberlist
        // cout << "JOINREP : from " << msg->addr->getAddress() << " to " << memberNode->addr.getAddress() << endl;
        push_member_list(msg);
        // 2. set memberNode->inGroup = true
        memberNode->inGroup = true;
    }
    else if(msg->msgType==PING){
        // 1. Check memberlist
        // cout << "PING : from " << msg->addr->getAddress() << " to " << memberNode->addr.getAddress() << endl;
        ping_handler(msg);
    }
    delete msg;
    return true;
}

/**
 * FUNCTION NAME: push_member_list
 *
 * DESCRIPTION: If a node does not exist in the memberList, it will be pushed to the memberList.
 */

//�ڵ�һ�������У���Ҫ������ǽ��յ��ļ�������JOINREQ��Ϣ������һ���½ڵ���Ҫ����ֲ�ʽϵͳʱ��
//������һ����������������ڵ�
void MP1Node::push_member_list(MessageHdr* msg) {
    // id, port, heartbeat, timestamp
    int id = 0;
    short port;
    memcpy(&id, &msg->addr->addr[0], sizeof(int));
    memcpy(&port, &msg->addr->addr[4], sizeof(short));
    long heartbeat = 1;
    long timestamp = this->par->getcurrtime();
    if (check_member_list(id, port) != nullptr)
        return;
    MemberListEntry e(id, port, heartbeat, timestamp);
    memberNode->memberList.push_back(e);
    Address* added = get_address(id, port);
    log->logNodeAdd(&memberNode->addr, added);
    delete added;
}


//�ڵڶ��������У��������ڴ����ڲ�״̬ͬ��ʱ���õģ�
//���磬��һ���ڵ�������ڵ���յ�������Ա�б����Ϣ��������JOINREP��������Ϣ���������͵�ͬ����Ϣ��
void MP1Node::push_member_list(MemberListEntry* e) {
    // ���ȼ���Ա�б����Ƿ��Ѵ��ڸýڵ�
    if (check_member_list(e->id, e->port) != nullptr) {
        // ����Ѵ��ڣ�������Ҫ����������Ŀ��ֱ�ӷ���
        return;
    }

    Address* addr = get_address(e->id, e->port);

    if (*addr == memberNode->addr) {
        delete addr;
        return;
    }

    if (par->getcurrtime() - e->timestamp < TREMOVE) {
        log->logNodeAdd(&memberNode->addr, addr);
        MemberListEntry new_entry = *e;
        memberNode->memberList.push_back(new_entry);
    }
    delete addr;
}

/**
 * FUNCTION NAME: get_address
 *
 * DESCRIPTION: return address
 */
Address* MP1Node::get_address(int id, short port) {
    Address* address = new Address();
    memcpy(&address->addr[0], &id, sizeof(int));//��id�ĵ�ַ��ʼ����4���ֽڵ����ݵ�address->addr������
    memcpy(&address->addr[4], &port, sizeof(short));
    return address;
}

/**
 * FUNCTION NAME: check_member_list
 *
 * DESCRIPTION: If the node exists in the memberList, the function will return pointer. Otherwise, the function will return false.
 */
MemberListEntry* MP1Node::check_member_list(int id, short port) {
    for (int i = 0; i < memberNode->memberList.size(); i++) {
        if (memberNode->memberList[i].id == id && memberNode->memberList[i].port == port)
            return &memberNode->memberList[i];//����һ��MemberNodeʵ��
        //����һ��ָ��ǰMemberNodeʵ���ĳ�Ա�б�(memberList)�е�i��Ԫ�ص�ָ�룬���Ԫ����һ��MemberListEntry����
        //MemberListEntry����ͨ�������˷ֲ�ʽϵͳ��ĳ���ڵ�ľ�����Ϣ����ڵ��ID���˿ںš�
        // ����ֵ�����һ�θ��µ�ʱ����ȡ�
    }
    return nullptr;
}

MemberListEntry* MP1Node::check_member_list(Address* node_addr) {
    for (int i = 0; i < memberNode->memberList.size(); i++) {
        int id = 0;
        short port = 0;
        memcpy(&id, &node_addr->addr[0], sizeof(int));
        memcpy(&port, &node_addr->addr[4], sizeof(short));
        if (memberNode->memberList[i].id == id && memberNode->memberList[i].port == port)
            return &memberNode->memberList[i];
    }
    return nullptr;
}


//�ڴ����У�MemberNode������һ�����ṹ���ʵ���������˽ڵ������״̬��Ϣ�������ַ�������������ȡ�
//MemberListEntry �Ƿֲ�ʽϵͳ�нڵ����ڼ�¼�����ڵ���Ϣ�����ݽṹ��ÿ��MemberListEntry��Ŀͨ��
// ������һ���ڵ�ı�ʶ������ID���������ַ�����������������һ�θ��µ�ʱ�������Ϣ��


/**
 * FUNCTION NAME: send_message
 *
 * DESCRIPTION: send message
 */
void MP1Node::send_message(Address* toaddr, MsgTypes t) {
    //auto msg = std::make_unique<MessageHdr>(); // ʹ������ָ���Զ������ڴ�
    MessageHdr* msg = new MessageHdr();
    //MessageHdr* msg = new MessageHdr();
    msg->msgType = t;
    msg->member_vector = memberNode->memberList;
    msg->addr = &memberNode->addr;
    emulNet->ENsend(&memberNode->addr, toaddr, (char*)msg, sizeof(MessageHdr));
    //ͨ��EmulNet����㷢����Ϣ��EmulNet��ģ�����绷����һ�������
    //�����ڽڵ�䴫����Ϣ�����д���������ǽ�msg������Ϊ������Ϣ�ӵ�ǰ�ڵ㷢�͵�Ŀ�ĵ�ַtoaddr��
}
//��������ı����ϵͳ��״̬��������˵���ǽڵ�A��״̬��ͨ�����ռ�����Ӧ��JOINREP����Ϣ���ڵ�A��֪�Լ��Ѿ�������������磬
// �����ܻ�ȡ�������������ڵ����Ϣ���������ڽڵ�A��ʼ���Լ��ĳ�Ա�б���ʼ�������е������ڵ����ͨ�š�


/**
 * FUNCTION NAME: ping_handler
 *ping_handler�����ĺ���Ŀ�������ý��յ���������Ϣ���±��صĳ�Ա�б���Ϣ����ͨ��������Ϣ��Я���ĳ�Ա��Ϣ����
    *ȷ�Ϸ���������Ϣ�Ľڵ���Ȼ��Ծ��
    *������֪�ڵ������״̬��
    *����·��ֵĽڵ㵽���س�Ա�б�
    * 
    * �����ڵ㷢������������Ϣ��
    *   ���ڷ��ͷ���������������Ϣ��֪ͨ�����ڵ��Լ���Ȼ��Ծ��
        ���ڽ��շ�������ǰ��MemberNode����ping_handler�������ڴ����յ������Է��ͽڵ��������Ϣ��

 * DESCRIPTION: The function handles the ping messages.
 */
void MP1Node::ping_handler(MessageHdr* msg) {
    update_src_member(msg);
    for (int i = 0; i < msg->member_vector.size(); i++) {
        // cout << " id : " << msg->member_vector[i].id << " , port : " << msg->member_vector[i].port << endl;
        if (msg->member_vector[i].id > 10 || msg->member_vector[i].id < 0) {
            assert(1 != 1);
        }
        MemberListEntry* node = check_member_list(msg->member_vector[i].id, msg->member_vector[i].port);
        //����membernode�е�ÿһ���е���Ŀ������
        if (node != nullptr) {
            if (msg->member_vector[i].heartbeat > node->heartbeat) {
                node->heartbeat = msg->member_vector[i].heartbeat;
                node->timestamp = par->getcurrtime();
            }
        }//��������ȡ���ģ�
        else {
            push_member_list(&msg->member_vector[i]);
        }//���û�е�ǰ�ڵ㣬���ֳ����뼴�ɣ�
    }
}


void MP1Node::update_src_member(MessageHdr* msg) {
    MemberListEntry* src_member = check_member_list(msg->addr);
    if (src_member != nullptr) {
        src_member->heartbeat++;
        src_member->timestamp = par->getcurrtime();
    }
    else {
        push_member_list(msg);
    }
}
//������Ŀ������
//��update_src_member�����У���һ����֪�Ľڵ㷢��������Ϣ����ǰ�ڵ�ʱ��
// ȷʵ�����Ӹýڵ��ڵ�ǰ�ڵ��Ա�б��м�¼������������src_member->heartbeat++����
//����������������Ӳ�������Ϊ�յ�������Ϣ������Զ����ӣ����ǵ�ǰ�ڵ��յ�������Ϣ���ֶ��������������
// ��Ϊ���յ�������һ�ּ�¼����Ӧ���ⷴӳ��һ�����ѡ�񣬼�����������Ϣ�����ٺ͸��½ڵ�Ļ�Ծ״̬��

//�����update_src_member�������������У�����Ľ���ȷʵ��ֱ����Է���������Ϣ�Ľڵ㣬
// �����Ǳ���������������ϢЯ���ĳ�Ա�б�msg->member_vector�������������Ŀ���Ǹ��»���ӷ���������Ϣ��
// �����Ϣ����ǰ�ڵ�ĳ�Ա�б��У������Ǵ���������Ϣ�и����������ڵ���Ϣ��

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
     * ��ʾ��Ծ״̬��ͨ��������������������������ڵ��ܹ��������ڵ�֤������Ȼ�ǻ�Ծ�ġ�
       ����������ᱻ�������������ڵ㷢�͵�������Ϣ�С�
       ���ϼ�⣺�����ڵ�ʹ������������������ڵ��Ƿ��Ծ�����һ���ڵ���һ��ʱ����û�и�
       ����������������û�з����µ�������Ϣ���������ܻᱻ��ΪʧЧ�����������ڵ�ĳ�Ա�б���
       �Ƴ���
	 */
     // Update heartbeat
    memberNode->heartbeat++;
    // Check TREMOVE

    //ѭ��������Ա�б����ÿ���ڵ���������ʱ��������ĳ���ڵ����ϴν��յ�����������ʱ�䳬
    // ����TREMOVE�������ֵ������ζ�Žڵ�����Ѿ�ʧ�ܻ��뿪�����磬�����Ҫ����ӳ�Ա�б���
    // �Ƴ���
    for (int i = memberNode->memberList.size() - 1; i >= 0; i--) {
        if (par->getcurrtime() - memberNode->memberList[i].timestamp >= TREMOVE) {
            Address* removed_addr = get_address(memberNode->memberList[i].id, memberNode->memberList[i].port);
            log->logNodeRemove(&memberNode->addr, removed_addr);
            memberNode->memberList.erase(memberNode->memberList.begin() + i);
            delete removed_addr;
        }
    }

    // Send PING to the members of memberList
    //�ٴα�����Ա�б���ÿ����Ա�ڵ㷢��������Ϣ�����Ƿֲ�ʽϵͳ�нڵ������໥���Ļ��ơ�
    //ͨ������������Ϣ���ڵ���Ը�֪�����е������ڵ�����Ȼ��Ծ��
    for (int i = 0; i < memberNode->memberList.size(); i++) {
        Address* address = get_address(memberNode->memberList[i].id, memberNode->memberList[i].port);
        send_message(address, PING);
        delete address;
    }

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
