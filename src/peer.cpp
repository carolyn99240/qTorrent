#include "peer.h"
#include "block.h"
#include "piece.h"
#include "torrent.h"
#include "torrentinfo.h"
#include "torrentmessage.h"
#include <QTcpSocket>
#include <QTimer>
#include <QDebug>

const int BLOCK_REQUEST_SIZE = 16384;
const int REPLY_TIMEOUT_MSEC = 5000;
const int HANDSHAKE_TIMEOUT_MSEC = 20000;
const int BLOCKS_TO_REQUEST = 5;
const int MAX_MESSAGE_LENGTH = 65536;

Peer::Peer(PeerType peerType) :
	m_torrent(nullptr),
	m_bitfield(nullptr),
	m_status(Created),
	m_peerType(peerType),
	m_socket(new QTcpSocket) {

	connectAll();
}

Peer::~Peer() {
	delete[] m_bitfield;
	delete m_socket;
}


void Peer::startConnection() {
	if(m_peerType == Client) {
		// That's not how it works
		// It's a client's duty to connect to you, not yours!
		qDebug() << "Error in Peer::startConnection(): called on Client Peer";
		return;
	}
	if(m_socket->isOpen()) {
		// Already connected/connecting
		qDebug() << "Error in Peer::startConnection(): Socket is open; m_status ==" << m_status;
		fatalError();
		return;
	}
	m_status = Connecting;
	qDebug() << "Connecting to" << addressPort();
	m_socket->connectToHost(m_address, m_port);
}

void Peer::cancelBlock(Block* block) {
	if(!m_socket->isOpen()) {
		// Do nothing if the socket is already closed
		return;
	}
	int	index = block->piece()->pieceNumber();
	int begin = block->begin();
	int length = block->size();
	TorrentMessage::cancel(m_socket, index, begin, length);
}

bool Peer::requestBlock() {
	if(m_peerType == Client) {
		// Can't request block from a client
		qDebug() << "Error in Peer:requestBlock(): called on Client Peer";
		return false;
	}
	if(!m_socket->isOpen()) {
		// Do nothing if the socket is closed;
		return false;
	}

	// Request block from Torrent object
	Block* block = m_torrent->requestBlock(this, BLOCK_REQUEST_SIZE);
	if(block == nullptr) {
		// Torrent object didn't give us a block to request
		return false;
	}

	// Send request
	int index = block->piece()->pieceNumber();
	int begin = block->begin();
	int length = block->size();
	TorrentMessage::request(m_socket, index, begin, length);

	// Start/Reset the replyTimeoutTimer
	m_replyTimeoutTimer.start();

	// Insert requested block into the queue
	m_blocksQueue.push_back(block);
	return true;
}

void Peer::disconnect() {
	qDebug() << "Disconnecting from" << addressPort();
	m_socket->close();
	// The finished() slot should be called automatically
}

void Peer::fatalError() {
	qDebug() << "Fatal error with" << addressPort() << "; Dropping connection";
	m_status = Error;
	m_socket->close();
}

Peer* Peer::createClient() {
	// TODO
	return nullptr;
}

Peer* Peer::createServer(Torrent *torrent, const QByteArray &address, int port) {
	Peer* peer = new Peer(Server);
	peer->initServer(torrent, address, port);
	return peer;
}


bool Peer::readHandshakeReply(bool *ok) {
	*ok = true;

	if(m_receivedDataBuffer.isEmpty()) {
		return false;
	}
	int i = 0;
	int protocolLength = m_receivedDataBuffer[i++];
	if(m_receivedDataBuffer.size() < 49 + protocolLength) {
		return false;
	}

	for(int j = 0; j < protocolLength; j++) {
		m_protocol.push_back(m_receivedDataBuffer[i++]);
	}
	for(int j = 0; j < 8; j++) {
		m_reserved.push_back(m_receivedDataBuffer[i++]);
	}
	for(int j = 0; j < 20; j++) {
		m_infoHash.push_back(m_receivedDataBuffer[i++]);
	}
	for(int j = 0; j < 20; j++) {
		m_peerId.push_back(m_receivedDataBuffer[i++]);
	}
	m_receivedDataBuffer.remove(0, 49 + protocolLength);

	if(m_infoHash != m_torrent->torrentInfo()->infoHash()) {
		// Info hash does not match the expected one
		qDebug() << "Info hash does not match expected one from peer" << addressPort();
		*ok = false;
		return false;
	}
	return true;
}

bool Peer::readPeerMessage(bool* ok) {
	*ok = true;

	// Smallest message (keep-alive) has a size of 4
	if(m_receivedDataBuffer.size() < 4) {
		return false;
	}

	// Counter for m_receivedDataBuffer
	int i = 0;

	// Calculate message length
	int length = 0;
	for(int j = 0; j < 4; j++) {
		length *= 256;
		length += (unsigned char)m_receivedDataBuffer[i++];
	}

	if(length > MAX_MESSAGE_LENGTH || length < 0) {
		*ok = false;
		return false;
	}

	if(length == 0) { // keep-alive
		qDebug() << addressPort() << ": keep-alive";
		m_receivedDataBuffer.remove(0, i);
		return true;
	}

	if(m_receivedDataBuffer.size() < 4 + length) {
		return false;
	}

	int messageId = m_receivedDataBuffer[i++];
	switch(messageId) {
	case TorrentMessage::Choke:
	{
		qDebug() << addressPort() << ": choke";
		m_peerChoking = true;
		break;
	}
	case TorrentMessage::Unchoke:
	{
		qDebug() << addressPort() << ": unchoke";
		m_peerChoking = false;
		break;
	}
	case TorrentMessage::Interested:
	{
		qDebug() << addressPort() << ": interested";
		m_peerInterested = true;
		break;
	}
	case TorrentMessage::NotInterested:
	{
		qDebug() << addressPort() << ": not interested";
		m_peerInterested = false;
		break;
	}
	case TorrentMessage::Have:
	{
		int pieceNumber = 0;
		for(int j = 0; j < 4; j++) {
			pieceNumber *= 256;
			pieceNumber += (unsigned char)m_receivedDataBuffer[i++];
		}
		m_bitfield[pieceNumber] = true;
		break;
	}
	case TorrentMessage::Bitfield:
	{
		int bitfieldSize = length - 1;
		if(bitfieldSize != m_torrent->torrentInfo()->bitfieldSize()) {
			qDebug() << "Error: Peer" << addressPort() << "sent bitfield of wrong size:" << bitfieldSize*8
					 << "expected" << m_torrent->torrentInfo()->bitfieldSize();
			*ok = false;
			return false;
		} else {
			for(int j = 0; j < bitfieldSize; j++) {
				unsigned char byte = m_receivedDataBuffer[i++];
				unsigned char pos = 0b10000000;
				for(int q = 0; q < 8; q++) {
					m_bitfield[j*8 + q] = ((byte & pos) != 0);
					pos = pos >> 1;
				}
			}
		}
		break;
	}
	case TorrentMessage::Request:
	{
		// TODO
		break;
	}
	case TorrentMessage::Piece:
	{
		int index = 0;
		int begin = 0;
		int blockLength = length - 9;
		for(int j = 0; j < 4; j++) {
			index *= 256;
			index += (unsigned char)m_receivedDataBuffer[i++];
		}
		for(int j = 0; j < 4; j++) {
			begin *= 256;
			begin += (unsigned char)m_receivedDataBuffer[i++];
		}
		Block* block = nullptr;
		int blockIndex = 0;
		for(auto b : m_blocksQueue) {
			if(b->piece()->pieceNumber() == index
					&& b->begin() == begin
					&& b->size() == blockLength) {
				block = b;
				break;
			}
			blockIndex++;
		}
		if(block == nullptr) {
			qDebug() << "Error: received unrequested block from peer" << addressPort()
					 << ". Block(" << index << begin << blockLength << ")";
		} else {
			m_timedOut = false;
			const char* blockData = m_receivedDataBuffer.data() + i;
			block->setData(this, blockData);
			m_blocksQueue.removeAt(blockIndex);
			if(m_blocksQueue.isEmpty()) {
				m_replyTimeoutTimer.stop();
			} else {
				m_replyTimeoutTimer.start();
			}
		}
		break;
	}
	case TorrentMessage::Cancel:
	{
		// TODO
		break;
	}
	case TorrentMessage::Port:
	{
		// TODO
		break;
	}
	default:
		qDebug() << "Error: Received unknown message with id =" << messageId
				 << " and length =" << length << "from" << addressPort();
		*ok = false;
		return false;
	}
	m_receivedDataBuffer.remove(0, 4 + length);
	return true;
}

void Peer::connectAll() {
	// Connection events
	connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(finished()));

	// Timeout callbacks
	connect(&m_replyTimeoutTimer, SIGNAL(timeout()), this, SLOT(replyTimeout()));
	connect(&m_handshakeTimeoutTimer, SIGNAL(timeout()), this, SLOT(handshakeTimeout()));

	// Timeout intervals
	m_replyTimeoutTimer.setInterval(REPLY_TIMEOUT_MSEC);
	m_handshakeTimeoutTimer.setInterval(HANDSHAKE_TIMEOUT_MSEC);
}

void Peer::initBitfield() {
	int bitfieldSize = m_torrent->torrentInfo()->bitfieldSize();
	m_bitfield = new bool[bitfieldSize*8];
	for(int i = 0; i < bitfieldSize*8; i++) {
		m_bitfield[i] = false;
	}
}

void Peer::initClient() {
	// TODO
}

void Peer::initServer(Torrent *torrent, const QByteArray &address, int port) {
	m_torrent = torrent;
	m_address = address;
	m_port = port;
	initBitfield();
	m_status = Created;
	m_amChoking = true;
	m_amInterested = false;
	m_peerChoking = true;
	m_peerInterested = false;
	m_receivedDataBuffer.clear();
	m_replyTimeoutTimer.stop();
	m_handshakeTimeoutTimer.stop();
	m_timedOut = false;
	m_blocksQueue.clear();
}

/* Slots */

void Peer::connected() {
	m_status = Handshaking;
	m_amChoking = true;
	m_amInterested = false;
	m_peerChoking = true;
	m_peerInterested = false;
	m_blocksQueue.clear();
	m_receivedDataBuffer.clear();
	m_timedOut = false;
	qDebug() << "Connected to" << addressPort();

	QByteArray dataToWrite;
	dataToWrite.push_back(char(19));
	dataToWrite.push_back("BitTorrent protocol");
	for(int i = 0; i < 8; i++) {
		dataToWrite.push_back(char(0));
	}
	dataToWrite.push_back(m_torrent->torrentInfo()->infoHash());
	dataToWrite.push_back("ThisIsNotAFakePeerId"); // TODO
	m_socket->write(dataToWrite);
	m_handshakeTimeoutTimer.start();
}

void Peer::readyRead() {
	m_receivedDataBuffer.push_back(m_socket->readAll());

	switch(m_status) {
	case Handshaking:
		bool ok;
		if(!readHandshakeReply(&ok)) {
			if(!ok) {
				fatalError();
			}
			break;
		}
		m_handshakeTimeoutTimer.stop();
		qDebug() << "Handshaking completed with peer" << addressPort();
		m_status = ConnectionEstablished;
		// Fall down
	case ConnectionEstablished:
		while(readPeerMessage(&ok));
		if(!ok) {
			fatalError();
			break;
		}

		if(!m_amInterested) {
			m_amInterested = true;
			TorrentMessage::interested(m_socket);
		} else if(!m_peerChoking) {
			while(m_blocksQueue.size() < BLOCKS_TO_REQUEST) {
				if(!requestBlock()) {
					break;
				}
			}
		}
		break;
	default:
		m_receivedDataBuffer.clear();
		break;
	}
}

void Peer::finished() {
	m_handshakeTimeoutTimer.stop();
	m_replyTimeoutTimer.stop();
	for(auto block : m_blocksQueue) {
		// Check if other peers have requested this block
		bool othersHaveBlock = false;
		for(Peer* peer : m_torrent->peers()) {
			if(peer == this) {
				continue;
			}
			if(peer->blocksQueue().contains(block)) {
				othersHaveBlock = true;
				break;
			}
		}
		// Delete block if nobody has requested it
		if(!othersHaveBlock) {
			block->piece()->deleteBlock(block);
		}
	}
	if(m_status != Error) {
		m_status = Disconnected;
	}
	m_blocksQueue.clear();
	qDebug() << "Connection to" << addressPort() << "closed" << m_socket->errorString();
}

void Peer::replyTimeout() {
	qDebug() << "Peer" << addressPort() << "took too long to reply";
	m_replyTimeoutTimer.stop();
	m_timedOut = true;
}

void Peer::handshakeTimeout() {
	qDebug() << "Peer" << addressPort() << "took too long to handshake";
	m_handshakeTimeoutTimer.stop();
}

/* Getter functions */

Torrent* Peer::torrent() {
	return m_torrent;
}

QByteArray& Peer::address() {
	return m_address;
}

int Peer::port() {
	return m_port;
}

bool* Peer::bitfield() {
	return m_bitfield;
}

QByteArray& Peer::protocol() {
	return m_protocol;
}

QByteArray& Peer::reserved() {
	return m_reserved;
}

QByteArray& Peer::infoHash() {
	return m_infoHash;
}

QByteArray& Peer::peerId() {
	return m_peerId;
}

Peer::Status Peer::status() {
	return m_status;
}

Peer::PeerType Peer::peerType() {
	return m_peerType;
}

bool Peer::amChoking() {
	return m_amChoking;
}

bool Peer::amInterested() {
	return m_amInterested;
}

bool Peer::peerChoking() {
	return m_peerChoking;
}

bool Peer::peerInterested() {
	return m_peerInterested;
}

QTcpSocket* Peer::socket() {
	return m_socket;
}

bool Peer::timedOut() {
	return m_timedOut;
}

QList<Block*>& Peer::blocksQueue() {
	return m_blocksQueue;
}

QString Peer::addressPort() {
	return QString(m_address) + ":" + QString::number(m_port);
}

bool Peer::hasPiece(int index) {
	return m_bitfield[index];
}
