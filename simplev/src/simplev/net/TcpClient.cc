/*
 * TcpClient.cc
 *
 *  Created on: Jun 24, 2014
 *      Author: damonhao
 */

#include <simplev/net/TcpClient.h>

#include <string>

#include <boost/bind.hpp>
#include <boost/get_pointer.hpp>

#include <simplev/base/Logging.h>
#include <simplev/net/Connector.h>
#include <simplev/net/EventLoop.h>
#include <simplev/net/SocketsOps.h>

using namespace simplev::net;

namespace simplev
{
namespace net
{
namespace detail
{
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
  loop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

}
}
}

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr)
	:loop_(CHECK_NOTNULL(loop)),
	 connector_(new Connector(loop, serverAddr)),
	 retry_(false),
	 connect_(true),
	 nextConnId_(1)
{
	connector_->setNewConnectionCallback(boost::bind(&TcpClient::newConnection, this, _1));
	LOG_INFO << "TcpClient::TcpClient[" << this << "] - connector " << get_pointer(connector_);
}

TcpClient::~TcpClient()
{
  LOG_INFO << "TcpClient::~TcpClient[" << this << "] - connector " << get_pointer(connector_);
	TcpConnectionPtr conn;
  {
    MutexLockGuard lock(mutex_);
    conn = connection_;
  }
  if(conn)
  {
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = boost::bind(&detail::removeConnection, loop_, _1);
    loop_->runInLoop(
        boost::bind(&TcpConnection::setCloseCallback, conn, cb));
  }
  else
  {
    connector_->stop();
    // FIXME: HACK
    loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));
  }
}


void TcpClient::newConnection(int sockfd)
{
	loop_->assertInLoopThread();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	std::string connName = buf;
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_, connName,sockfd,localAddr,peerAddr));
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(boost::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
  {
    MutexLockGuard lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::connect()
{
  // FIXME: check state
  LOG_INFO << "TcpClient::connect[" << this << "] - connecting to "
  		<< connector_->serverAddress().toIpPort();
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect()
{
  connect_ = false;

  {
    MutexLockGuard lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());
  {
    MutexLockGuard lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }
  loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
  if(connect_ && retry_)
  {
  	LOG_INFO << "TcpClient::connect[" << this << "] - Reconnecting to "
  	             << connector_->serverAddress().toIpPort();
  	connector_->restart();
  }
}
