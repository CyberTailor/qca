/*
 * qca_securelayer.cpp - Qt Cryptographic Architecture
 * Copyright (C) 2003-2005  Justin Karneges <justin@affinix.com>
 * Copyright (C) 2004,2005  Brad Hards <bradh@frogmouth.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "qca_securelayer.h"

#include <QtCore>
//#include <qhostaddress.h>
#include "qcaprovider.h"

namespace QCA {

//----------------------------------------------------------------------------
// SecureFilter
//----------------------------------------------------------------------------
SecureFilter::~SecureFilter()
{
}

bool SecureFilter::isClosable() const
{
	return false;
}

bool SecureFilter::haveClosed() const
{
	return false;
}

void SecureFilter::close()
{
}

QSecureArray SecureFilter::readUnprocessed()
{
	return QSecureArray();
}

//----------------------------------------------------------------------------
// SecureLayer
//----------------------------------------------------------------------------
SecureLayer::SecureLayer(QObject *parent)
:QObject(parent)
{
	_signals = true;
}

void SecureLayer::setStatefulOnly(bool b)
{
	_signals = !b;
}

void SecureLayer::layerUpdateBegin()
{
	_read = bytesAvailable();
	_readout = bytesOutgoingAvailable();
	_closed = haveClosed();
	_error = !ok();
}

void SecureLayer::layerUpdateEnd()
{
	if(_signals)
	{
		if(_read > bytesAvailable())
			QTimer::singleShot(0, this, SIGNAL(readyRead()));
		if(_readout > bytesOutgoingAvailable())
			QTimer::singleShot(0, this, SIGNAL(readyReadOutgoing()));
		if(!_closed && haveClosed())
			QTimer::singleShot(0, this, SIGNAL(closed()));
		if(!_error && !ok())
			QTimer::singleShot(0, this, SIGNAL(error()));
	}
}

//----------------------------------------------------------------------------
// TLS
//----------------------------------------------------------------------------
class TLS::Private
{
public:
	Private()
	{
		//store = 0;
	}

	void reset()
	{
		handshaken = false;
		closing = false;
		in.resize(0);
		out.resize(0);
		from_net.resize(0);
		to_net.resize(0);
		host = QString();
		hostMismatch = false;
		cert = Certificate();
		bytesEncoded = 0;
		tryMore = false;
	}

	void appendArray(QByteArray *a, const QByteArray &b)
	{
		int oldsize = a->size();
		a->resize(oldsize + b.size());
		memcpy(a->data() + oldsize, b.data(), b.size());
	}

	Certificate cert;
	Validity certValidity;
	TLSContext *c;
	QByteArray in, out, to_net, from_net;
	int bytesEncoded;
	bool tryMore;
	bool handshaken;
	QString host;
	bool hostMismatch;
	bool closing;
	Error errorCode;

	Certificate ourCert;
	PrivateKey ourKey;
	//Store *store;
};

TLS::TLS(QObject *parent, const QString &provider)
:SecureLayer(parent), Algorithm("tls", provider)
{
	d = new Private;
	d->c = (TLSContext *)context();
}

TLS::~TLS()
{
	delete d;
}

void TLS::reset()
{
	d->reset();
	// TODO: d->c->reset ??
}

QStringList TLS::supportedCipherSuites(const QString &provider)
{
	Q_UNUSED(provider);
	return QStringList();
}

void TLS::setCertificate(const CertificateChain &cert, const PrivateKey &key)
{
	Q_UNUSED(cert);
	//d->ourCert = cert;
	d->ourKey = key;
}

void TLS::setTrustedCertificates(const CertificateCollection &trusted)
{
	Q_UNUSED(trusted);
	//d->store = new Store(store);
}

void TLS::setConstraints(SecurityLevel s)
{
	Q_UNUSED(s);
}

void TLS::setConstraints(int, int)
{
}

void TLS::setConstraints(const QStringList &cipherSuiteList)
{
	Q_UNUSED(cipherSuiteList);
}

bool TLS::canCompress(const QString &provider)
{
	Q_UNUSED(provider);
	return false;
}

void TLS::setCompressionEnabled(bool b)
{
	Q_UNUSED(b);
}

bool TLS::startClient(const QString &host)
{
	d->reset();
	d->host = host;

	/*if(!d->c->startClient(*((StoreContext *)d->store->context()), *((CertContext *)d->ourCert.context()), *((PKeyContext *)d->ourKey.context())))
		return false;
	QTimer::singleShot(0, this, SLOT(update()));*/
	return true;
}

bool TLS::startServer()
{
	d->reset();

	/*if(!d->c->startServer(*((StoreContext *)d->store->context()), *((CertContext *)d->ourCert.context()), *((PKeyContext *)d->ourKey.context())))
		return false;
	QTimer::singleShot(0, this, SLOT(update()));*/
	return true;
}

bool TLS::isHandshaken() const
{
	return d->handshaken;
}

bool TLS::isCompressed() const
{
	return false;
}

TLS::Version TLS::version() const
{
	return TLS_v1;
}

QString TLS::cipherSuite() const
{
	return QString();
}

int TLS::cipherBits() const
{
	return 0;
}

int TLS::cipherMaxBits() const
{
	return 0;
}

TLS::Error TLS::errorCode() const
{
	return d->errorCode;
}

TLS::IdentityResult TLS::peerIdentityResult() const
{
	if(d->cert.isNull())
		return NoCert;

	//if(d->certValidity != QCA::Valid)
	//	return BadCert;

	if(d->hostMismatch)
		return HostMismatch;

	return Valid;
}

Validity TLS::peerCertificateValidity() const
{
	return d->certValidity;
}

CertificateChain TLS::localCertificateChain() const
{
	return CertificateChain();
	//return d->ourCert;
}

CertificateChain TLS::peerCertificateChain() const
{
	return CertificateChain();
	//return d->cert;
}

bool TLS::isClosable() const
{
	return true;
}

bool TLS::haveClosed() const
{
	return false;
}

bool TLS::ok() const
{
	return false;
}

int TLS::bytesAvailable() const
{
	return 0;
}

int TLS::bytesOutgoingAvailable() const
{
	return 0;
}

void TLS::close()
{
	if(!d->handshaken || d->closing)
		return;

	d->closing = true;
	QTimer::singleShot(0, this, SLOT(update()));
}

void TLS::write(const QSecureArray &a)
{
	d->appendArray(&d->out, a.toByteArray());
	//update();
}

QSecureArray TLS::read()
{
	QByteArray a = d->in;
	d->in.resize(0);
	return a;
}

void TLS::writeIncoming(const QByteArray &a)
{
	d->appendArray(&d->from_net, a);
	//update();
}

QByteArray TLS::readOutgoing(int *plainBytes)
{
	Q_UNUSED(plainBytes);
	QByteArray a = d->to_net;
	d->to_net.resize(0);
	return a;
}

QSecureArray TLS::readUnprocessed()
{
	QByteArray a = d->from_net;
	d->from_net.resize(0);
	return a;
}

//----------------------------------------------------------------------------
// SASL
//----------------------------------------------------------------------------
QString *saslappname = 0;
class SASL::Private
{
public:
	void setSecurityProps()
	{
		c->setSecurityProps(noPlain, noActive, noDict, noAnon, reqForward, reqCreds, reqMutual, ssfmin, ssfmax, ext_authid, ext_ssf);
	}

	// security opts
	bool noPlain, noActive, noDict, noAnon, reqForward, reqCreds, reqMutual;
	int ssfmin, ssfmax;
	QString ext_authid;
	int ext_ssf;

	bool tried;
	SASLContext *c;
	//QHostAddress localAddr, remoteAddr;
	int localPort, remotePort;
	QByteArray stepData;
	bool allowCSF;
	bool first, server;
	Error errorCode;

	QByteArray inbuf, outbuf;
};

SASL::SASL(QObject *parent, const QString &provider)
:SecureLayer(parent), Algorithm("sasl", provider)
{
	d = new Private;
	d->c = (SASLContext *)context();
	reset();
}

SASL::~SASL()
{
	delete d;
}

void SASL::reset()
{
	d->localPort = -1;
	d->remotePort = -1;

	d->noPlain = false;
	d->noActive = false;
	d->noDict = false;
	d->noAnon = false;
	d->reqForward = false;
	d->reqCreds = false;
	d->reqMutual = false;
	d->ssfmin = 0;
	d->ssfmax = 0;
	d->ext_authid = QString();
	d->ext_ssf = 0;

	d->inbuf.resize(0);
	d->outbuf.resize(0);

	d->c->reset();
}

SASL::Error SASL::errorCode() const
{
	return d->errorCode;
}

SASL::AuthCondition SASL::authCondition() const
{
	return (AuthCondition)d->c->authError();
}

void SASL::setConstraints(AuthFlags f, SecurityLevel s)
{
	Q_UNUSED(f);
	Q_UNUSED(s);

	/*d->noPlain    = (f & SAllowPlain) ? false: true;
	d->noAnon     = (f & SAllowAnonymous) ? false: true;
	//d->noActive   = (f & SAllowActiveVulnerable) ? false: true;
	//d->noDict     = (f & SAllowDictVulnerable) ? false: true;
	d->reqForward = (f & SRequireForwardSecrecy) ? true : false;
	d->reqCreds   = (f & SRequirePassCredentials) ? true : false;
	d->reqMutual  = (f & SRequireMutualAuth) ? true : false;*/

	//d->ssfmin = minSSF;
	//d->ssfmax = maxSSF;
}

void SASL::setConstraints(AuthFlags, int, int)
{
}

void SASL::setExternalAuthId(const QString &authid)
{
	d->ext_authid = authid;
}

void SASL::setExternalSSF(int x)
{
	d->ext_ssf = x;
}

void SASL::setLocalAddr(const QString &addr, quint16 port)
{
	Q_UNUSED(addr);
	//d->localAddr = addr;
	d->localPort = port;
}

void SASL::setRemoteAddr(const QString &addr, quint16 port)
{
	Q_UNUSED(addr);
	//d->remoteAddr = addr;
	d->remotePort = port;
}

bool SASL::startClient(const QString &service, const QString &host, const QStringList &mechlist, ClientSendMode)
{
	SASLContext::HostPort la, ra;
	/*if(d->localPort != -1) {
		la.addr = d->localAddr;
		la.port = d->localPort;
	}
	if(d->remotePort != -1) {
		ra.addr = d->remoteAddr;
		ra.port = d->remotePort;
	}*/

	//d->allowCSF = allowClientSendFirst;
	d->c->setCoreProps(service, host, d->localPort != -1 ? &la : 0, d->remotePort != -1 ? &ra : 0);
	d->setSecurityProps();

	if(!d->c->clientStart(mechlist))
		return false;
	d->first = true;
	d->server = false;
	d->tried = false;
	QTimer::singleShot(0, this, SLOT(tryAgain()));
	return true;
}

bool SASL::startServer(const QString &service, const QString &host, const QString &realm, QStringList *mechlist, ServerSendMode)
{
	//Q_UNUSED(allowServerSendLast);

	SASLContext::HostPort la, ra;
	/*if(d->localPort != -1) {
		la.addr = d->localAddr;
		la.port = d->localPort;
	}
	if(d->remotePort != -1) {
		ra.addr = d->remoteAddr;
		ra.port = d->remotePort;
	}*/

	d->c->setCoreProps(service, host, d->localPort != -1 ? &la : 0, d->remotePort != -1 ? &ra : 0);
	d->setSecurityProps();

	QString appname;
	if(saslappname)
		appname = *saslappname;
	else
		appname = "qca";

	if(!d->c->serverStart(realm, mechlist, appname))
		return false;
	d->first = true;
	d->server = true;
	d->tried = false;
	return true;
}

void SASL::putServerFirstStep(const QString &mech)
{
	/*int r =*/ d->c->serverFirstStep(mech, 0);
	//handleServerFirstStep(r);
}

void SASL::putServerFirstStep(const QString &mech, const QByteArray &clientInit)
{
	/*int r =*/ d->c->serverFirstStep(mech, &clientInit);
	//handleServerFirstStep(r);
}

void SASL::putStep(const QByteArray &stepData)
{
	d->stepData = stepData;
	//tryAgain();
}

void SASL::setUsername(const QString &user)
{
	d->c->setClientParams(&user, 0, 0, 0);
}

void SASL::setAuthzid(const QString &authzid)
{
	d->c->setClientParams(0, &authzid, 0, 0);
}

void SASL::setPassword(const QSecureArray &pass)
{
	d->c->setClientParams(0, 0, &pass, 0);
}

void SASL::setRealm(const QString &realm)
{
	d->c->setClientParams(0, 0, 0, &realm);
}

void SASL::continueAfterParams()
{
	//tryAgain();
}

void SASL::continueAfterAuthCheck()
{
	//tryAgain();
}

int SASL::ssf() const
{
	return d->c->security();
}

bool SASL::ok() const
{
	return false;
}

int SASL::bytesAvailable() const
{
	return 0;
}

int SASL::bytesOutgoingAvailable() const
{
	return 0;
}

void SASL::close()
{
}

void SASL::write(const QSecureArray &a)
{
	Q_UNUSED(a);
}

QSecureArray SASL::read()
{
	return QSecureArray();
}

void SASL::writeIncoming(const QByteArray &a)
{
	Q_UNUSED(a);
}

QByteArray SASL::readOutgoing(int *plainBytes)
{
	Q_UNUSED(plainBytes);
	return QByteArray();
}

}
