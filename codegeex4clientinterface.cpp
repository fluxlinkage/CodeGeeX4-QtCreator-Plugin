#include "codegeex4clientinterface.h"

#include "codegeex4settings.h"
#include <QJsonDocument>
#include <QNetworkReply>

namespace CodeGeeX4 {
namespace Internal {

QMap<QString,QString> CodeGeeX4ClientInterface::m_langMap;

CodeGeeX4ClientInterface::CodeGeeX4ClientInterface()
    :LanguageClient::BaseClientInterface()
{
    if(m_langMap.isEmpty()){
        m_langMap.insert("abap","Abap");
        m_langMap.insert("c","C");
        m_langMap.insert("cpp","C++");
        m_langMap.insert("csharp","C#");
        m_langMap.insert("css","CSS");
        m_langMap.insert("dart","Dart");
        m_langMap.insert("dockerfile", "Dockerfile");
        m_langMap.insert("elixir","Elixir");
        m_langMap.insert("erlang","Erlang");
        m_langMap.insert("fsharp","F#");
        m_langMap.insert("go","Go");
        m_langMap.insert("groovy","Groovy");
        m_langMap.insert("html","HTML");
        m_langMap.insert("java","Java");
        m_langMap.insert("javascript","JavaScript");
        m_langMap.insert("lua","Lua");
        m_langMap.insert("markdown","Markdown");
        m_langMap.insert("objective-c","Objective-C");
        m_langMap.insert("objective-cpp","Objective-C++");
        m_langMap.insert("perl","Perl");
        m_langMap.insert("php","PHP");
        m_langMap.insert("powershell","PowerShell");
        m_langMap.insert("python","Python");
        m_langMap.insert("r","R");
        m_langMap.insert("ruby","Ruby");
        m_langMap.insert("rust","Rust");
        m_langMap.insert("scala","Scala");
        m_langMap.insert("shellscript","Shell");
        m_langMap.insert("sql","SQL");
        m_langMap.insert("swift","Swift");
        m_langMap.insert("typescript","TypeScript");
        m_langMap.insert("tex","TeX");
        m_langMap.insert("vb","Visual Basic");
    }
}

CodeGeeX4ClientInterface::~CodeGeeX4ClientInterface()
{
}

Utils::FilePath CodeGeeX4ClientInterface::serverDeviceTemplate() const
{
    return "";
}

void CodeGeeX4ClientInterface::replyFinished()
{
    if(m_reply==nullptr){
        return;
    }
    QByteArray qba=m_reply->readAll();
    m_reply->disconnect();
    m_reply=nullptr;
    if(qba.isEmpty()){
        QJsonObject errorObj;
        errorObj.insert("code",-32603);
        errorObj.insert("message","Request failed!");
        QJsonObject obj;
        obj.insert("id",m_id);
        obj.insert("error",errorObj);
        LanguageServerProtocol::JsonRpcMessage errMsg(obj);
        emit messageReceived(errMsg);
        return;
    }
    QJsonParseError err;
    QJsonDocument doc=QJsonDocument::fromJson(qba,&err);
    if(err.error!=QJsonParseError::NoError){
        QJsonObject errorObj;
        errorObj.insert("code",-32603);
        errorObj.insert("message","Request failed!");
        QJsonObject obj;
        obj.insert("id",m_id);
        obj.insert("error",errorObj);
        LanguageServerProtocol::JsonRpcMessage errMsg(obj);
        emit messageReceived(errMsg);
    }else{
        QJsonObject obj=doc.object();
        QJsonArray ary=obj.value("choices").toArray();
        QJsonObject obj2=ary.first().toObject();
        QString str=obj2.value("text").toString();
        QJsonObject responseRangeObj;
        responseRangeObj.insert("start",m_position);
        responseRangeObj.insert("end",m_position);
        QJsonObject responseSubObj;
        responseSubObj.insert("position",m_position);
        responseSubObj.insert("range",responseRangeObj);
        responseSubObj.insert("text",str);
        responseSubObj.insert("displayText",str);
        responseSubObj.insert("uuid",QUuid::createUuid().toString());
        QJsonArray responseAry;
        responseAry.push_back(responseSubObj);
        QJsonObject objResult;
        objResult.insert("completions",responseAry);
        QJsonObject responseObj;
        responseObj.insert("id",m_id);
        responseObj.insert("result",objResult);
        LanguageServerProtocol::JsonRpcMessage responseMsg(responseObj);
        emit messageReceived(responseMsg);
    }
}

void CodeGeeX4ClientInterface::sendData(const QByteArray &data)
{
    m_writeBuffer.open(QBuffer::Append);
    m_writeBuffer.write(data);
    m_writeBuffer.close();
    LanguageServerProtocol::BaseMessage baseMsg;
    QString parseError;
    m_writeBuffer.open(QBuffer::ReadWrite);
    LanguageServerProtocol::BaseMessage::parse(&m_writeBuffer,parseError,baseMsg);
    m_writeBuffer.close();
    if(baseMsg.isValid()){
        if(baseMsg.isComplete()){
            LanguageServerProtocol::JsonRpcMessage msg(baseMsg);
            QJsonObject objSend=msg.toJsonObject();
            if(objSend.value("method")=="initialize"){
                QJsonObject InfoObj;
                InfoObj.insert("name","Fake server for CodeGeeX4");
                InfoObj.insert("version","0.1");
                QJsonObject capObj;
                capObj.insert("completionProvider",QJsonObject());
                capObj.insert("textDocumentSync",0);
                QJsonObject objResult;
                objResult.insert("capabilities",capObj);
                objResult.insert("serverInfo",InfoObj);
                QJsonObject obj;
                obj.insert("id",objSend.value("id"));
                obj.insert("result",objResult);
                LanguageServerProtocol::JsonRpcMessage responseMsg(obj);
                emit messageReceived(responseMsg);
            }else if(objSend.value("method")=="shutdown"){
                clearReply();
                QJsonObject obj;
                obj.insert("id",objSend.value("id"));
                obj.insert("result",QJsonValue());
                LanguageServerProtocol::JsonRpcMessage responseMsg(obj);
                emit messageReceived(responseMsg);
            }else if(objSend.value("method")=="textDocument/didOpen"){
                QJsonObject docParams=objSend.value("params").toObject().value("textDocument").toObject();
                QString langCode=m_langMap.value(docParams.value("languageId").toString(),"None");
                if(langCode!="None"){
                    m_fileLang.insert(docParams.value("uri").toString(),langCode);
                }
            }else if(objSend.value("method")=="getCompletionsCycling"){
                clearReply();
                QJsonObject objParams=objSend.value("params").toObject();
                QString uriStr=objParams.value("doc").toObject().value("uri").toString();
                QString filePath=QUrl(uriStr).toLocalFile();
                QFileInfo fileInfo(filePath);
                QDir fileDir=fileInfo.dir();
                QString langCode=m_fileLang.value(uriStr,"None");
                QUrl url(settings().url.value());
                QNetworkRequest req(url);
                req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
                //QJsonArray ary;
                m_pos=objParams.value("pos").toInt();
                m_position=objParams.value("doc").toObject().value("position");
                QString origTxt=objParams.value("txt").toString();
                QString contextPrefix=origTxt.left(m_pos);
                QString contextSuffix=origTxt.mid(m_pos);
                // if(CodeGeeX4Settings::instance().braceBalance.value()){
                //     m_braceLevel=origTxt.count('{')-origTxt.count('}');
                // }
                int maxLen=settings().contextLimit.value();
                bool trimmed=false;
                if(contextPrefix.length()+contextSuffix.length()>maxLen){
                    trimmed=true;
                    int offset=contextPrefix.length()+contextSuffix.length()-maxLen;
                    int offsetPrefix=offset/2;
                    int indN=contextPrefix.indexOf('\n',offsetPrefix);
                    if(indN>=0){
                        offsetPrefix=indN+1;
                    }else{
                        int indSpace=contextPrefix.indexOf(' ',offsetPrefix);
                        if(indSpace>=0){
                            offsetPrefix=indSpace+1;
                        }
                    }
                    m_pos-=offsetPrefix;
                    contextPrefix=contextPrefix.mid(offsetPrefix);
                    int offsetSuffix=offset-offsetPrefix;
                    indN=contextSuffix.lastIndexOf('\n',contextSuffix.length()-1-offsetSuffix);
                    if(indN>=0){
                        offsetSuffix=indN;
                    }else{
                        int indSpace=contextPrefix.lastIndexOf(' ',contextSuffix.length()-1-offsetSuffix);
                        if(indSpace>=0){
                            offsetSuffix=indSpace;
                        }
                    }
                    contextPrefix=contextPrefix.left(offsetSuffix);
                }
                QString prompt;
                if(langCode=="C"||langCode=="C++"){
                    if(!trimmed&&settings().expandHeaders.value()){
                        QUrl fileURI(uriStr);
                        if(fileURI.isLocalFile()){
                            static const QRegularExpression reHeaderQ("#include\\s+\"([^\"]+)\"");
                            static const QRegularExpression reHeaderA("#include\\s+<([^>]+)>");
                            //QFileInfo qfi(fileURI.toLocalFile());
                            QStringList searchPaths;
                            QDir dir(fileDir);
                            searchPaths.push_back(dir.absolutePath());
                            while(dir.cdUp()){
                                searchPaths.push_back(dir.absolutePath());
                            }
                            QStringList headerNames;
                            //QStringList headerStrings;
                            {
                                QRegularExpressionMatchIterator it=reHeaderQ.globalMatch(contextPrefix);
                                while(it.hasNext()){
                                    QRegularExpressionMatch match=it.next();
                                    QString header=match.captured(1);
                                    if(header.isEmpty()){
                                        continue;
                                    }
                                    headerNames.push_back(header);
                                    //headerStrings.push_back(match.captured(0));
                                }
                            }
                            {
                                QRegularExpressionMatchIterator it=reHeaderA.globalMatch(contextPrefix);
                                while(it.hasNext()){
                                    QRegularExpressionMatch match=it.next();
                                    QString header=match.captured(1);
                                    if(header.isEmpty()){
                                        continue;
                                    }
                                    headerNames.push_back(header);
                                    //headerStrings.push_back(match.captured(0));
                                }
                            }
                            int space=maxLen-contextPrefix.length()-contextSuffix.length();
                            bool full=false;
                            for(int i=0;i<headerNames.size();i++){
                                const QString &nm=headerNames.at(i);
                                QFileInfo tmpFI(nm);
                                if(tmpFI.baseName()==fileInfo.baseName()){
                                    for(const QString &dirPath:searchPaths){
                                        QFileInfo qfiHeader(dirPath+"/"+nm);
                                        if(qfiHeader.exists()){
                                            if(!expandHeader(prompt,dirPath+"/"+nm,fileDir,space,m_pos)){
                                                i=headerNames.size();
                                                full=true;
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                            if(!full){
                                for(int i=0;i<headerNames.size();i++){
                                    const QString &nm=headerNames.at(i);
                                    QFileInfo tmpFI(nm);
                                    if(tmpFI.baseName()==fileInfo.baseName()){
                                        continue;
                                    }
                                    for(const QString &dirPath:searchPaths){
                                        QFileInfo qfiHeader(dirPath+"/"+nm);
                                        if(qfiHeader.exists()){
                                            if(!expandHeader(prompt,dirPath+"/"+nm,fileDir,space,m_pos)){
                                                i=headerNames.size();
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                m_row=0;
                m_col=0;
                for(int i=0;i<m_pos;i++){
                    if(contextPrefix.at(i).unicode()=='\n'){
                        m_row++;
                        m_col=0;
                    }else{
                        m_col++;
                    }
                }
                //QString prompt;
                //上下文
                prompt+="<|user|>\n###PATH:"+fileInfo.fileName()+"\n";
                if(langCode!="None"){
                    prompt+="###LANGUAGE:"+langCode+"\n";
                }else{
                    prompt+="###LANGUAGE:\n";
                }
                prompt += "###MODE:BLOCK\n<|code_suffix|>"+contextSuffix;
                prompt += "<|code_prefix|>"+contextPrefix;
                prompt += "<|code_middle|><|assistant|>\n";
                QJsonObject request_object;
                request_object.insert("prompt",prompt);
                request_object.insert("max_tokens",settings().length.value());
                // request_object.insert("top_p",settings().topP.value());
                // request_object.insert("top_k",settings().topK.value());
                // request_object.insert("temperarure",settings().temperarure.value());
                m_id=objSend.value("id");
                if(m_manager==nullptr){
                    m_manager=QSharedPointer<QNetworkAccessManager>(new QNetworkAccessManager());
                }
                // QFile qf("D:/temp/CodeGeeX4.txt");
                // qf.open(QIODevice::Append|QIODevice::Text);
                // qf.write(QJsonDocument(request_object).toJson());
                // qf.write("\n");
                // qf.close();
                //qWarning("%d",QJsonDocument(request_object).toJson().constData());
                m_reply=QSharedPointer<QNetworkReply>(m_manager->post(req,QJsonDocument(request_object).toJson()));
                connect(m_reply.get(),&QNetworkReply::finished,this,&CodeGeeX4ClientInterface::replyFinished);
            }
            QByteArray &bufferRaw=m_writeBuffer.buffer();
            bufferRaw.remove(0,bufferRaw.indexOf(baseMsg.header())+baseMsg.header().size()+baseMsg.contentLength);
        }
    }else{
        QJsonObject errorObj;
        errorObj.insert("code",-32700);
        errorObj.insert("message",parseError);
        QJsonObject obj;
        obj.insert("id",QJsonValue());
        obj.insert("error",errorObj);
        LanguageServerProtocol::JsonRpcMessage errMsg(obj);
        emit messageReceived(errMsg);
    }
}

void CodeGeeX4ClientInterface::clearReply()
{
    if(m_reply!=nullptr){
        m_reply->disconnect();
        m_reply=nullptr;
        QJsonObject errorObj;
        errorObj.insert("code",-32603);
        errorObj.insert("message","Request canceled.");
        QJsonObject obj;
        obj.insert("id",m_id);
        obj.insert("error",errorObj);
        LanguageServerProtocol::JsonRpcMessage errMsg(obj);
        emit messageReceived(errMsg);
    }
}

bool CodeGeeX4ClientInterface::expandHeader(QString &txt, const QString &path, const QDir &baseDir, int &space, int &pos)
{
    QFileInfo qfiHeader(path);
    if(qfiHeader.size()>space){
        return false;
    }
    // int ind=txt.indexOf(includeStr);
    // if(ind<0){
    //     return false;
    // }
    QFile qf(path);
    QTextStream qts(&qf);
    qf.open(QIODevice::ReadOnly|QIODevice::Text);
    QString content=qts.readAll();
    qf.close();
    //txt.replace(ind,includeStr.length(),content);
    //space-=(content.length()-includeStr.length());
    //pos+=(content.length()-includeStr.length());
    space-=content.length();
    txt+="###REFERENCE:\n###PATH:"+baseDir.relativeFilePath(path)+"\n"+content;
    if(!content.endsWith('\n')){
        txt+="\n";
    }
    return true;
}

} // namespace Internal
} // namespace CodeGeeX4
