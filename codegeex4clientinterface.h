#ifndef CODEGEEX2_INTERNAL_CODEGEEX2CLIENTINTERFACE_H
#define CODEGEEX2_INTERNAL_CODEGEEX2CLIENTINTERFACE_H

#include <languageclient/languageclientinterface.h>

#include <QNetworkAccessManager>

class QNetworkReply;

namespace CodeGeeX4 {
namespace Internal {

class CodeGeeX4ClientInterface : public LanguageClient::BaseClientInterface
{
public:
    CodeGeeX4ClientInterface();
    ~CodeGeeX4ClientInterface();

    Utils::FilePath serverDeviceTemplate() const override;

public slots:
    void replyFinished();

protected:
    void sendData(const QByteArray &data) override;
private:
    QBuffer m_writeBuffer;
    QSharedPointer<QNetworkReply> m_reply;
    QJsonValue m_id;
    int m_pos;
    QJsonValue m_position;
    int m_row;
    int m_col;
    int m_braceLevel;
    QMap<QString,QString> m_fileLang;

    QSharedPointer<QNetworkAccessManager> m_manager;

    void clearReply();
    bool expandHeader(QString &txt, const QString &path,const QDir &baseDir, int &space, int &pos);

    static QMap<QString,QString> m_langMap;
};

} // namespace Internal
} // namespace CodeGeeX4

#endif // CODEGEEX2_INTERNAL_CODEGEEX2CLIENTINTERFACE_H
