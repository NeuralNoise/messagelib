/*
  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include "composerviewbase.h"

#include "attachment/attachmentcontrollerbase.h"
#include "attachment/attachmentmodel.h"
#include "composer/signaturecontroller.h"
#include "composer-ng/richtextcomposerng.h"
#include <KPIMTextEdit/RichTextComposerImages>
#include "composer-ng/richtextcomposersignatures.h"
#include "KPIMTextEdit/RichTextComposerControler"
#include "job/emailaddressresolvejob.h"
#include "composer/keyresolver.h"
#include "part/globalpart.h"
#include "utils/kleo_util.h"
#include "part/infopart.h"
#include "composer.h"
#include "utils/util.h"
#include "utils/util_p.h"
#include "imagescaling/imagescalingutils.h"

#include "SendLater/SendLaterInfo"
#include "SendLater/SendLaterUtil"

#include <libkdepim/recentaddresses.h>
#include "helper/messagehelper.h"

#include <MessageComposer/RecipientsEditor>
#include "settings/messagecomposersettings.h"

#include <MessageViewer/ObjectTreeEmptySource>
#include <MimeTreeParser/ObjectTreeParser>
#ifndef QT_NO_CURSOR
#include <Libkdepim/KCursorSaver>
#endif

#include <sonnet/dictionarycombobox.h>
#include <KIdentityManagement/Identity>

#include <MessageCore/StringUtil>
#include <MessageCore/NodeHelper>

#include <mailtransport/transportcombobox.h>
#include <mailtransportakonadi/messagequeuejob.h>
#include <mailtransport/transportmanager.h>

#include <Akonadi/KMime/SpecialMailCollections>
#include <AkonadiCore/itemcreatejob.h>
#include <AkonadiCore/collectionfetchjob.h>
#include <AkonadiWidgets/collectioncombobox.h>
#include <Akonadi/KMime/MessageFlags>

#include <KIdentityManagement/kidentitymanagement/identitycombo.h>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <KEmailAddress>

#include <QSaveFile>
#include <KLocalizedString>
#include <KMessageBox>
#include <QTextBlock>
#include "messagecomposer_debug.h"

#include <QDir>
#include <QTimer>
#include <QUuid>
#include <QTextCodec>
#include <QStandardPaths>
#include <followupreminder/followupremindercreatejob.h>

using namespace MessageComposer;

static QStringList encodeIdn(const QStringList &emails)
{
    QStringList encoded;
    encoded.reserve(emails.count());
    for (const QString &email : emails) {
        encoded << KEmailAddress::normalizeAddressesAndEncodeIdn(email);
    }

    return encoded;
}

ComposerViewBase::ComposerViewBase(QObject *parent, QWidget *parentGui)
    : QObject(parent)
    , m_msg(KMime::Message::Ptr(new KMime::Message))
    , m_attachmentController(nullptr)
    , m_attachmentModel(nullptr)
    , m_signatureController(nullptr)
    , m_recipientsEditor(nullptr)
    , m_identityCombo(nullptr)
    , m_identMan(nullptr)
    , m_editor(nullptr)
    , m_transport(nullptr)
    , m_dictionary(nullptr)
    , m_fccCombo(nullptr)
    , m_parentWidget(parentGui)
    , m_sign(false)
    , m_encrypt(false)
    , m_neverEncrypt(false)
    , m_mdnRequested(false)
    , m_urgent(false)
    , m_cryptoMessageFormat(Kleo::AutoFormat)
    , m_pendingQueueJobs(0)
    , m_autoSaveTimer(nullptr)
    , m_autoSaveErrorShown(false)
    , m_autoSaveInterval(1 * 1000 * 60)   // default of 1 min
    , mSendLaterInfo(nullptr)
{
    m_charsets << "utf-8"; // default, so we have a backup in case client code forgot to set.

    initAutoSave();

}

ComposerViewBase::~ComposerViewBase()
{
    delete mSendLaterInfo;
}

bool ComposerViewBase::isComposing() const
{
    return !m_composers.isEmpty();
}

void ComposerViewBase::setMessage(const KMime::Message::Ptr &msg, bool allowDecryption)
{
    if (m_attachmentModel)  {
        foreach (const MessageCore::AttachmentPart::Ptr &attachment, m_attachmentModel->attachments()) {
            m_attachmentModel->removeAttachment(attachment);
        }
    }
    m_msg = msg;
    if (m_recipientsEditor) {
        m_recipientsEditor->clear();
        m_recipientsEditor->setRecipientString(m_msg->to()->mailboxes(), MessageComposer::Recipient::To);
        m_recipientsEditor->setRecipientString(m_msg->cc()->mailboxes(), MessageComposer::Recipient::Cc);
        m_recipientsEditor->setRecipientString(m_msg->bcc()->mailboxes(), MessageComposer::Recipient::Bcc);
        m_recipientsEditor->setFocusBottom();

        // If we are loading from a draft, load unexpanded aliases as well
        if (auto hrd = m_msg->headerByType("X-KMail-UnExpanded-To")) {
            const QStringList spl = hrd->asUnicodeString().split(QLatin1Char(','));
            for (const QString &addr : spl) {
                m_recipientsEditor->addRecipient(addr, MessageComposer::Recipient::To);
            }
        }
        if (auto hrd = m_msg->headerByType("X-KMail-UnExpanded-CC")) {
            const QStringList spl = hrd->asUnicodeString().split(QLatin1Char(','));
            for (const QString &addr : spl) {
                m_recipientsEditor->addRecipient(addr, MessageComposer::Recipient::Cc);
            }
        }
        if (auto hrd = m_msg->headerByType("X-KMail-UnExpanded-BCC")) {
            const QStringList spl = hrd->asUnicodeString().split(QLatin1Char(','));
            for (const QString &addr : spl) {
                m_recipientsEditor->addRecipient(addr, MessageComposer::Recipient::Bcc);
            }
        }
    }

    // First, we copy the message and then parse it to the object tree parser.
    // The otp gets the message text out of it, in textualContent(), and also decrypts
    // the message if necessary.
    KMime::Content *msgContent = new KMime::Content;
    msgContent->setContent(m_msg->encodedContent());
    msgContent->parse();
    MessageViewer::EmptySource emptySource;
    MimeTreeParser::ObjectTreeParser otp(&emptySource);  //All default are ok
    emptySource.setAllowDecryption(allowDecryption);
    otp.parseObjectTree(msgContent);

    // Load the attachments
    foreach (const auto &att, otp.nodeHelper()->attachmentsOfExtraContents()) {
        addAttachmentPart(att);
    }
    foreach (const auto &att, msgContent->attachments()) {
        addAttachmentPart(att);
    }

    int transportId = -1;
    if (auto hdr = m_msg->headerByType("X-KMail-Transport")) {
        transportId = hdr->asUnicodeString().toInt();
    }

    if (m_transport) {
        const MailTransport::Transport *transport = MailTransport::TransportManager::self()->transportById(transportId);
        if (transport) {
            m_transport->setCurrentTransport(transport->id());
        }
    }

    // Set the HTML text and collect HTML images
    QString htmlContent = otp.htmlContent();
    if (htmlContent.isEmpty()) {
        m_editor->setPlainText(otp.plainTextContent());
    } else {
        //Bug 372085 <div id="name"> is replaced in qtextedit by <a id="name">... => break url
        htmlContent.replace(QRegularExpression(QStringLiteral("<div\\s*id=\".*\">")), QStringLiteral("<div>"));
        m_editor->setHtml(htmlContent);
        Q_EMIT enableHtml();
        collectImages(m_msg.data());
    }

    if (auto hdr = m_msg->headerByType("X-KMail-CursorPos")) {
        m_editor->setCursorPositionFromStart(hdr->asUnicodeString().toInt());
    }
    delete msgContent;
}

void ComposerViewBase::updateTemplate(const KMime::Message::Ptr &msg)
{
    // First, we copy the message and then parse it to the object tree parser.
    // The otp gets the message text out of it, in textualContent(), and also decrypts
    // the message if necessary.
    KMime::Content *msgContent = new KMime::Content;
    msgContent->setContent(msg->encodedContent());
    msgContent->parse();
    MessageViewer::EmptySource emptySource;
    MimeTreeParser::ObjectTreeParser otp(&emptySource);  //All default are ok
    otp.parseObjectTree(msgContent);
    // Set the HTML text and collect HTML images
    if (!otp.htmlContent().isEmpty()) {
        m_editor->setHtml(otp.htmlContent());
        Q_EMIT enableHtml();
        collectImages(msg.data());
    } else {
        m_editor->setPlainText(otp.plainTextContent());
    }

    if (auto hdr = msg->headerByType("X-KMail-CursorPos")) {
        m_editor->setCursorPositionFromStart(hdr->asUnicodeString().toInt());
    }
    delete msgContent;
}

void ComposerViewBase::saveMailSettings()
{
    const KIdentityManagement::Identity identity = identityManager()->identityForUoid(m_identityCombo->currentIdentity());
    auto header = new KMime::Headers::Generic("X-KMail-Transport");
    header->fromUnicodeString(QString::number(m_transport->currentTransportId()), "utf-8");
    m_msg->setHeader(header);

    header = new KMime::Headers::Generic("X-KMail-Fcc");
    header->fromUnicodeString(QString::number(m_fccCollection.id()), "utf-8");
    m_msg->setHeader(header);
    header = new KMime::Headers::Generic("X-KMail-Identity");
    header->fromUnicodeString(QString::number(identity.uoid()), "utf-8");
    m_msg->setHeader(header);
    header = new KMime::Headers::Generic("X-KMail-Dictionary");
    header->fromUnicodeString(m_dictionary->currentDictionary(), "utf-8");
    m_msg->setHeader(header);

    // Save the quote prefix which is used for this message. Each message can have
    // a different quote prefix, for example depending on the original sender.
    if (m_editor->quotePrefixName().isEmpty()) {
        m_msg->removeHeader("X-KMail-QuotePrefix");
    } else {
        header = new KMime::Headers::Generic("X-KMail-QuotePrefix");
        header->fromUnicodeString(m_editor->quotePrefixName(), "utf-8");
        m_msg->setHeader(header);
    }

    if (m_editor->composerControler()->isFormattingUsed()) {
        qCDebug(MESSAGECOMPOSER_LOG) << "HTML mode";
        header = new KMime::Headers::Generic("X-KMail-Markup");
        header->fromUnicodeString(QStringLiteral("true"), "utf-8");
        m_msg->setHeader(header);
    } else {
        m_msg->removeHeader("X-KMail-Markup");
        qCDebug(MESSAGECOMPOSER_LOG) << "Plain text";
    }

}

void ComposerViewBase::clearFollowUp()
{
    mFollowUpDate = QDate();
    mFollowUpCollection = Akonadi::Collection();
}

void ComposerViewBase::send(MessageComposer::MessageSender::SendMethod method, MessageComposer::MessageSender::SaveIn saveIn, bool checkMailDispatcher)
{
    mSendMethod = method;
    mSaveIn = saveIn;

#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif

    const KIdentityManagement::Identity identity = identityManager()->identityForUoid(m_identityCombo->currentIdentity());

    if (identity.attachVcard() && m_attachmentController->attachOwnVcard()) {
        const QString vcardFileName = identity.vCardFile();
        if (!vcardFileName.isEmpty()) {
            m_attachmentController->addAttachmentUrlSync(QUrl::fromLocalFile(vcardFileName));
        }
    }
    saveMailSettings();

    if (m_editor->composerControler()->isFormattingUsed() && inlineSigningEncryptionSelected()) {
        const QString keepBtnText = m_encrypt ?
                                    m_sign ? i18n("&Keep markup, do not sign/encrypt")
                                    : i18n("&Keep markup, do not encrypt")
                                    : i18n("&Keep markup, do not sign");
        const QString yesBtnText = m_encrypt ?
                                   m_sign ? i18n("Sign/Encrypt (delete markup)")
                                   : i18n("Encrypt (delete markup)")
                                   : i18n("Sign (delete markup)");
        int ret = KMessageBox::warningYesNoCancel(m_parentWidget,
                  i18n("<qt><p>Inline signing/encrypting of HTML messages is not possible;</p>"
                       "<p>do you want to delete your markup?</p></qt>"),
                  i18n("Sign/Encrypt Message?"),
                  KGuiItem(yesBtnText),
                  KGuiItem(keepBtnText));
        if (KMessageBox::Cancel == ret) {
            return;
        }
        if (KMessageBox::No == ret) {
            m_encrypt = false;
            m_sign = false;
        } else {
            Q_EMIT disableHtml(NoConfirmationNeeded);
        }
    }

    if (m_neverEncrypt && saveIn != MessageComposer::MessageSender::SaveInNone) {
        // we can't use the state of the mail itself, to remember the
        // signing and encryption state, so let's add a header instead
        auto header = new KMime::Headers::Generic("X-KMail-SignatureActionEnabled");
        header->fromUnicodeString(m_sign ? QStringLiteral("true") : QStringLiteral("false"), "utf-8");
        m_msg->setHeader(header);
        header = new KMime::Headers::Generic("X-KMail-EncryptActionEnabled");
        header->fromUnicodeString(m_encrypt ? QStringLiteral("true") : QStringLiteral("false"), "utf-8");
        m_msg->setHeader(header);
        header = new KMime::Headers::Generic("X-KMail-CryptoMessageFormat");
        header->fromUnicodeString(QString::number(m_cryptoMessageFormat), "utf-8");
        m_msg->setHeader(header);
    } else {
        MessageComposer::Util::removeNotNecessaryHeaders(m_msg);
    }

    if (mSendMethod == MessageComposer::MessageSender::SendImmediate  && checkMailDispatcher) {
        MessageComposer::Util::sendMailDispatcherIsOnline(m_parentWidget);
    }

    readyForSending();
}

void ComposerViewBase::setCustomHeader(const QMap<QByteArray, QString> &customHeader)
{
    m_customHeader = customHeader;
}

void ComposerViewBase::readyForSending()
{
    qCDebug(MESSAGECOMPOSER_LOG) << "Entering readyForSending";
    if (!m_msg) {
        qCDebug(MESSAGECOMPOSER_LOG) << "m_msg == 0!";
        return;
    }

    if (!m_composers.isEmpty()) {
        // This may happen if e.g. the autosave timer calls applyChanges.
        qCDebug(MESSAGECOMPOSER_LOG) << "Called while composer active; ignoring.";
        return;
    }

    // first, expand all addresses
    MessageComposer::EmailAddressResolveJob *job = new MessageComposer::EmailAddressResolveJob(this);
    const KIdentityManagement::Identity identity = identityManager()->identityForUoid(m_identityCombo->currentIdentity());
    if (!identity.isNull()) {
        job->setDefaultDomainName(identity.defaultDomainName());
    }
    job->setFrom(from());
    job->setTo(m_recipientsEditor->recipientStringList(MessageComposer::Recipient::To));
    job->setCc(m_recipientsEditor->recipientStringList(MessageComposer::Recipient::Cc));
    job->setBcc(m_recipientsEditor->recipientStringList(MessageComposer::Recipient::Bcc));
    connect(job, &MessageComposer::EmailAddressResolveJob::result, this, &ComposerViewBase::slotEmailAddressResolved);
    job->start();
}

void ComposerViewBase::slotEmailAddressResolved(KJob *job)
{
    if (job->error()) {
        qCWarning(MESSAGECOMPOSER_LOG) << "An error occurred while resolving the email addresses:" << job->errorString();
        // This error could be caused by a broken search infrastructure, so we ignore it for now
        // to not block sending emails completely.
    }

    bool autoresizeImage = MessageComposer::MessageComposerSettings::self()->autoResizeImageEnabled();

    const MessageComposer::EmailAddressResolveJob *resolveJob = qobject_cast<MessageComposer::EmailAddressResolveJob *>(job);
    if (mSaveIn == MessageComposer::MessageSender::SaveInNone) {
        mExpandedFrom = resolveJob->expandedFrom();
        mExpandedTo = resolveJob->expandedTo();
        mExpandedCc = resolveJob->expandedCc();
        mExpandedBcc = resolveJob->expandedBcc();
        if (autoresizeImage) {
            QStringList listEmails;
            listEmails << mExpandedFrom;
            listEmails << mExpandedTo;
            listEmails << mExpandedCc;
            listEmails << mExpandedBcc;
            MessageComposer::Utils resizeUtils;
            autoresizeImage = resizeUtils.filterRecipients(listEmails);
        }

    } else { // saved to draft, so keep the old values, not very nice.
        mExpandedFrom = from();
        foreach (const MessageComposer::Recipient::Ptr &r, m_recipientsEditor->recipients()) {
            switch (r->type()) {
            case MessageComposer::Recipient::To: mExpandedTo << r->email(); break;
            case MessageComposer::Recipient::Cc: mExpandedCc << r->email(); break;
            case MessageComposer::Recipient::Bcc: mExpandedBcc << r->email(); break;
            case MessageComposer::Recipient::Undefined: Q_ASSERT(!"Unknown recpient type!"); break;
            }
        }
        QStringList unExpandedTo, unExpandedCc, unExpandedBcc;
        foreach (const QString &exp, resolveJob->expandedTo()) {
            if (!mExpandedTo.contains(exp)) {  // this address was expanded, so save it explicitly
                unExpandedTo << exp;
            }
        }
        foreach (const QString &exp, resolveJob->expandedCc()) {
            if (!mExpandedCc.contains(exp)) {
                unExpandedCc << exp;
            }
        }
        foreach (const QString &exp, resolveJob->expandedBcc()) {
            if (!mExpandedBcc.contains(exp)) {  // this address was expanded, so save it explicitly
                unExpandedBcc << exp;
            }
        }
        auto header = new KMime::Headers::Generic("X-KMail-UnExpanded-To");
        header->from7BitString(unExpandedTo.join(QStringLiteral(", ")).toLatin1());
        m_msg->setHeader(header);
        header = new KMime::Headers::Generic("X-KMail-UnExpanded-CC");
        header->from7BitString(unExpandedCc.join(QStringLiteral(", ")).toLatin1());
        m_msg->setHeader(header);
        header = new KMime::Headers::Generic("X-KMail-UnExpanded-BCC");
        header->from7BitString(unExpandedBcc.join(QStringLiteral(", ")).toLatin1());
        m_msg->setHeader(header);
        autoresizeImage = false;
    }

    Q_ASSERT(m_composers.isEmpty()); //composers should be empty. The caller of this function
    //checks for emptyness before calling it
    //so just ensure it actually is empty
    //and document it
    // we first figure out if we need to create multiple messages with different crypto formats
    // if so, we create a composer per format
    // if we aren't signing or encrypting, this just returns a single empty message
    bool wasCanceled = false;
    if (m_neverEncrypt && mSaveIn != MessageComposer::MessageSender::SaveInNone && !mSendLaterInfo) {
        MessageComposer::Composer *composer = new MessageComposer::Composer;
        composer->setNoCrypto(true);
        m_composers.append(composer);
    } else {
        m_composers = generateCryptoMessages(wasCanceled);
        if (wasCanceled) {
            return;
        }
    }

    if (m_composers.isEmpty()) {
        Q_EMIT failed(i18n("It was not possible to create a message composer."));
        return;
    }

    if (autoresizeImage) {
        if (MessageComposer::MessageComposerSettings::self()->askBeforeResizing()) {
            if (m_attachmentModel) {
                MessageComposer::Utils resizeUtils;
                if (resizeUtils.containsImage(m_attachmentModel->attachments())) {
                    const int rc = KMessageBox::warningYesNo(m_parentWidget, i18n("Do you want to resize images?"),
                                   i18n("Auto Resize Images"), KStandardGuiItem::yes(), KStandardGuiItem::no());
                    if (rc == KMessageBox::Yes) {
                        autoresizeImage = true;
                    } else {
                        autoresizeImage = false;
                    }
                } else {
                    autoresizeImage = false;
                }
            }
        }
    }
    // Compose each message and prepare it for queueing, sending, or storing
    foreach (MessageComposer::Composer *composer, m_composers) {
        fillGlobalPart(composer->globalPart());
        m_editor->fillComposerTextPart(composer->textPart());
        fillInfoPart(composer->infoPart(), UseExpandedRecipients);

        if (m_attachmentModel) {
            composer->addAttachmentParts(m_attachmentModel->attachments(), autoresizeImage);
        }

        connect(composer, &MessageComposer::Composer::result, this, &ComposerViewBase::slotSendComposeResult);
        composer->start();
        qCDebug(MESSAGECOMPOSER_LOG) << "Started a composer for sending!";

    }
}

namespace
{

// helper methods for reading encryption settings

inline int encryptKeyNearExpiryWarningThresholdInDays()
{
    if (! MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire()) {
        return -1;
    }
    const int num =
        MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrKeyNearExpiryThresholdDays();
    return qMax(1, num);
}

inline int signingKeyNearExpiryWarningThresholdInDays()
{
    if (! MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire()) {
        return -1;
    }
    const int num =
        MessageComposer::MessageComposerSettings::self()->cryptoWarnSignKeyNearExpiryThresholdDays();
    return qMax(1, num);
}

inline int encryptRootCertNearExpiryWarningThresholdInDays()
{
    if (! MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire()) {
        return -1;
    }
    const int num =
        MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrRootNearExpiryThresholdDays();
    return qMax(1, num);
}

inline int signingRootCertNearExpiryWarningThresholdInDays()
{
    if (! MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire()) {
        return -1;
    }
    const int num =
        MessageComposer::MessageComposerSettings::self()->cryptoWarnSignRootNearExpiryThresholdDays();
    return qMax(1, num);
}

inline int encryptChainCertNearExpiryWarningThresholdInDays()
{
    if (! MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire()) {
        return -1;
    }
    const int num =
        MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrChaincertNearExpiryThresholdDays();
    return qMax(1, num);
}

inline int signingChainCertNearExpiryWarningThresholdInDays()
{
    if (! MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire()) {
        return -1;
    }
    const int num =
        MessageComposer::MessageComposerSettings::self()->cryptoWarnSignChaincertNearExpiryThresholdDays();;
    return qMax(1, num);
}

inline bool encryptToSelf()
{
    return MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelf();
}

inline bool showKeyApprovalDialog()
{
    return MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApproval();
}

} // nameless namespace

QList< MessageComposer::Composer * > ComposerViewBase::generateCryptoMessages(bool &wasCanceled)
{
    const KIdentityManagement::Identity &id = m_identMan->identityForUoidOrDefault(m_identityCombo->currentIdentity());

    qCDebug(MESSAGECOMPOSER_LOG) << "filling crypto info";
    Kleo::KeyResolver *keyResolver = new Kleo::KeyResolver(encryptToSelf(),
            showKeyApprovalDialog(),
            id.pgpAutoEncrypt(),
            m_cryptoMessageFormat,
            encryptKeyNearExpiryWarningThresholdInDays(),
            signingKeyNearExpiryWarningThresholdInDays(),
            encryptRootCertNearExpiryWarningThresholdInDays(),
            signingRootCertNearExpiryWarningThresholdInDays(),
            encryptChainCertNearExpiryWarningThresholdInDays(),
            signingChainCertNearExpiryWarningThresholdInDays());

    QStringList encryptToSelfKeys;
    QStringList signKeys;

    bool signSomething = m_sign;
    bool doSignCompletely = m_sign;
    bool encryptSomething = m_encrypt;
    bool doEncryptCompletely = m_encrypt;

    //Add encryptionkeys from id to keyResolver
    qCDebug(MESSAGECOMPOSER_LOG) << id.pgpEncryptionKey().isEmpty() << id.smimeEncryptionKey().isEmpty();
    if (!id.pgpEncryptionKey().isEmpty()) {
        encryptToSelfKeys.push_back(QLatin1String(id.pgpEncryptionKey()));
    }
    if (!id.smimeEncryptionKey().isEmpty()) {
        encryptToSelfKeys.push_back(QLatin1String(id.smimeEncryptionKey()));
    }
    if (keyResolver->setEncryptToSelfKeys(encryptToSelfKeys) != Kleo::Ok) {
        qCDebug(MESSAGECOMPOSER_LOG) << "Failed to set encryptoToSelf keys!";
        return QList< MessageComposer::Composer * >();
    }

    //Add signingkeys from id to keyResolver
    if (!id.pgpSigningKey().isEmpty()) {
        signKeys.push_back(QLatin1String(id.pgpSigningKey()));
    }
    if (!id.smimeSigningKey().isEmpty()) {
        signKeys.push_back(QLatin1String(id.smimeSigningKey()));
    }
    if (keyResolver->setSigningKeys(signKeys) != Kleo::Ok) {
        qCDebug(MESSAGECOMPOSER_LOG) << "Failed to set signing keys!";
        return QList< MessageComposer::Composer * >();
    }

    if (m_attachmentModel) {
        foreach (const MessageCore::AttachmentPart::Ptr &attachment, m_attachmentModel->attachments()) {
            if (attachment->isSigned()) {
                signSomething = true;
            } else {
                doEncryptCompletely = false;
            }
            if (attachment->isEncrypted()) {
                encryptSomething = true;
            } else {
                doSignCompletely = false;
            }
        }
    }

    QStringList recipients(mExpandedTo), bcc(mExpandedBcc);
    recipients.append(mExpandedCc);

    keyResolver->setPrimaryRecipients(recipients);
    keyResolver->setSecondaryRecipients(bcc);

    bool result = true;
    bool canceled = false;
    signSomething = determineWhetherToSign(doSignCompletely, keyResolver, signSomething, result, canceled);
    if (!result) {
        // TODO handle failure
        qCDebug(MESSAGECOMPOSER_LOG) << "determineWhetherToSign: failed to resolve keys! oh noes";
        if (!canceled) {
            Q_EMIT failed(i18n("Failed to resolve keys. Please report a bug."));
        } else {
            Q_EMIT failed(QString());
        }
        wasCanceled = canceled;
        return QList< MessageComposer::Composer *>();
    }

    canceled = false;
    encryptSomething = determineWhetherToEncrypt(doEncryptCompletely, keyResolver, encryptSomething, signSomething, result, canceled);
    if (!result) {
        // TODO handle failure
        qCDebug(MESSAGECOMPOSER_LOG) << "determineWhetherToEncrypt: failed to resolve keys! oh noes";
        if (!canceled) {
            Q_EMIT failed(i18n("Failed to resolve keys. Please report a bug."));
        } else {
            Q_EMIT failed(QString());
        }

        wasCanceled = canceled;
        return QList< MessageComposer::Composer *>();
    }

    //No encryption or signing is needed
    if (!signSomething && !encryptSomething) {
        return QList< MessageComposer::Composer * >() << new MessageComposer::Composer();
    }

    const Kleo::Result kpgpResult = keyResolver->resolveAllKeys(signSomething, encryptSomething);
    if (kpgpResult == Kleo::Canceled) {
        qCDebug(MESSAGECOMPOSER_LOG) << "resolveAllKeys: one key resolution canceled by user";
        return QList< MessageComposer::Composer *>();
    } else if (kpgpResult != Kleo::Ok) {
        // TODO handle failure
        qCDebug(MESSAGECOMPOSER_LOG) << "resolveAllKeys: failed to resolve keys! oh noes";
        Q_EMIT failed(i18n("Failed to resolve keys. Please report a bug."));
        return QList< MessageComposer::Composer *>();
    }
    qCDebug(MESSAGECOMPOSER_LOG) << "done resolving keys:";

    QList< MessageComposer::Composer * > composers;

    if (encryptSomething || signSomething) {
        Kleo::CryptoMessageFormat concreteFormat = Kleo::AutoFormat;
        for (unsigned int i = 0; i < numConcreteCryptoMessageFormats; ++i) {
            concreteFormat = concreteCryptoMessageFormats[i];
            if (keyResolver->encryptionItems(concreteFormat).empty()) {
                continue;
            }

            if (!(concreteFormat & m_cryptoMessageFormat)) {
                continue;
            }

            MessageComposer::Composer *composer =  new MessageComposer::Composer;

            if (encryptSomething) {
                std::vector<Kleo::KeyResolver::SplitInfo> encData = keyResolver->encryptionItems(concreteFormat);
                std::vector<Kleo::KeyResolver::SplitInfo>::iterator it;
                std::vector<Kleo::KeyResolver::SplitInfo>::iterator end(encData.end());
                QList<QPair<QStringList, std::vector<GpgME::Key> > > data;
                data.reserve(encData.size());
                for (it = encData.begin(); it != end; ++it) {
                    QPair<QStringList, std::vector<GpgME::Key> > p(it->recipients, it->keys);
                    data.append(p);
                    qCDebug(MESSAGECOMPOSER_LOG) << "got resolved keys for:" << it->recipients;
                }
                composer->setEncryptionKeys(data);
            }

            if (signSomething) {
                // find signing keys for this format
                std::vector<GpgME::Key> signingKeys = keyResolver->signingKeys(concreteFormat);
                composer->setSigningKeys(signingKeys);
            }

            composer->setMessageCryptoFormat(concreteFormat);
            composer->setSignAndEncrypt(signSomething, encryptSomething);

            composers.append(composer);
        }
    } else {
        MessageComposer::Composer *composer =  new MessageComposer::Composer;
        composers.append(composer);
        //If we canceled sign or encrypt be sure to change status in attachment.
        markAllAttachmentsForSigning(false);
        markAllAttachmentsForEncryption(false);
    }

    if (composers.isEmpty() && (signSomething || encryptSomething)) {
        Q_ASSERT_X(false, "ComposerViewBase::fillCryptoInfo", "No concrete sign or encrypt method selected");
    }

    return composers;
}

void ComposerViewBase::fillGlobalPart(MessageComposer::GlobalPart *globalPart)
{
    globalPart->setParentWidgetForGui(m_parentWidget);
    globalPart->setCharsets(m_charsets);
    globalPart->setMDNRequested(m_mdnRequested);
}

void ComposerViewBase::fillInfoPart(MessageComposer::InfoPart *infoPart, ComposerViewBase::RecipientExpansion expansion)
{
    // TODO splitAddressList and expandAliases ugliness should be handled by a
    // special AddressListEdit widget... (later: see RecipientsEditor)

    if (m_fccCombo) {
        infoPart->setFcc(QString::number(m_fccCombo->currentCollection().id()));
    } else {
        if (m_fccCollection.isValid()) {
            infoPart->setFcc(QString::number(m_fccCollection.id()));
        }
    }

    infoPart->setTransportId(m_transport->currentTransportId());
    infoPart->setReplyTo(replyTo());
    if (expansion == UseExpandedRecipients) {
        infoPart->setFrom(mExpandedFrom);
        infoPart->setTo(mExpandedTo);
        infoPart->setCc(mExpandedCc);
        infoPart->setBcc(mExpandedBcc);
    } else {
        infoPart->setFrom(from());
        infoPart->setTo(m_recipientsEditor->recipientStringList(MessageComposer::Recipient::To));
        infoPart->setCc(m_recipientsEditor->recipientStringList(MessageComposer::Recipient::Cc));
        infoPart->setBcc(m_recipientsEditor->recipientStringList(MessageComposer::Recipient::Bcc));
    }
    infoPart->setSubject(subject());
    infoPart->setUserAgent(QStringLiteral("KMail"));
    infoPart->setUrgent(m_urgent);

    if (m_msg->inReplyTo()) {
        infoPart->setInReplyTo(m_msg->inReplyTo()->asUnicodeString());
    }

    if (m_msg->references()) {
        infoPart->setReferences(m_msg->references()->asUnicodeString());
    }

    KMime::Headers::Base::List extras;
    if (auto hdr = m_msg->headerByType("X-KMail-SignatureActionEnabled")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-EncryptActionEnabled")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-CryptoMessageFormat")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-UnExpanded-To")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-UnExpanded-CC")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-UnExpanded-BCC")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->organization(false)) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Identity")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Transport")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Fcc")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Drafts")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Templates")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Link-Message")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Link-Type")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-Face")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-FccDisabled")) {
        extras << hdr;
    }
    if (auto hdr = m_msg->headerByType("X-KMail-Dictionary")) {
        extras << hdr;
    }

    infoPart->setExtraHeaders(extras);
}

void ComposerViewBase::slotSendComposeResult(KJob *job)
{
    qCDebug(MESSAGECOMPOSER_LOG) << "compose job might have error error" << job->error() << "errorString" << job->errorString();
    Q_ASSERT(dynamic_cast< MessageComposer::Composer * >(job));
    MessageComposer::Composer *composer = static_cast< MessageComposer::Composer * >(job);

    if (composer->error() == MessageComposer::Composer::NoError) {
        Q_ASSERT(m_composers.contains(composer));
        // The messages were composed successfully.
        qCDebug(MESSAGECOMPOSER_LOG) << "NoError.";
        const int numberOfMessage(composer->resultMessages().size());
        for (int i = 0; i < numberOfMessage; ++i) {
            if (mSaveIn == MessageComposer::MessageSender::SaveInNone) {
                queueMessage(composer->resultMessages().at(i), composer);
            } else {
                saveMessage(composer->resultMessages().at(i), mSaveIn);
            }
        }
        saveRecentAddresses(composer->resultMessages().at(0));
    } else if (composer->error() == MessageComposer::Composer::UserCancelledError) {
        // The job warned the user about something, and the user chose to return
        // to the message.  Nothing to do.
        qCDebug(MESSAGECOMPOSER_LOG) << "UserCancelledError.";
        Q_EMIT failed(i18n("Job cancelled by the user"));
    } else {
        qCDebug(MESSAGECOMPOSER_LOG) << "other Error.";
        QString msg;
        if (composer->error() == MessageComposer::Composer::BugError) {
            msg = i18n("Could not compose message: %1 \n Please report this bug.", job->errorString());
        } else {
            msg = i18n("Could not compose message: %1", job->errorString());
        }
        Q_EMIT failed(msg);
    }

    m_composers.removeAll(composer);
}

void ComposerViewBase::saveRecentAddresses(const KMime::Message::Ptr &msg)
{
    foreach (const QByteArray &address, msg->to()->addresses()) {
        KPIM::RecentAddresses::self(MessageComposer::MessageComposerSettings::self()->config())->add(QLatin1String(address));
    }
    foreach (const QByteArray &address, msg->cc()->addresses()) {
        KPIM::RecentAddresses::self(MessageComposer::MessageComposerSettings::self()->config())->add(QLatin1String(address));
    }
    foreach (const QByteArray &address, msg->bcc()->addresses()) {
        KPIM::RecentAddresses::self(MessageComposer::MessageComposerSettings::self()->config())->add(QLatin1String(address));
    }
}

void ComposerViewBase::queueMessage(const KMime::Message::Ptr &message, MessageComposer::Composer *composer)
{
    const MessageComposer::InfoPart *infoPart = composer->infoPart();
    MailTransport::MessageQueueJob *qjob = new MailTransport::MessageQueueJob(this);
    qjob->setMessage(message);
    qjob->transportAttribute().setTransportId(infoPart->transportId());
    if (mSendMethod == MessageComposer::MessageSender::SendLater) {
        qjob->dispatchModeAttribute().setDispatchMode(MailTransport::DispatchModeAttribute::Manual);
    }

    if (message->hasHeader("X-KMail-FccDisabled")) {
        qjob->sentBehaviourAttribute().setSentBehaviour(MailTransport::SentBehaviourAttribute::Delete);
    } else if (!infoPart->fcc().isEmpty()) {
        qjob->sentBehaviourAttribute().setSentBehaviour(MailTransport::SentBehaviourAttribute::MoveToCollection);

        const Akonadi::Collection sentCollection(infoPart->fcc().toLongLong());
        qjob->sentBehaviourAttribute().setMoveToCollection(sentCollection);
    } else {
        qjob->sentBehaviourAttribute().setSentBehaviour(
            MailTransport::SentBehaviourAttribute::MoveToDefaultSentCollection);
    }
    MessageComposer::Util::addSendReplyForwardAction(message, qjob);

    fillQueueJobHeaders(qjob, message, infoPart);

    MessageCore::StringUtil::removePrivateHeaderFields(message, false);

    QMapIterator<QByteArray, QString> customHeader(m_customHeader);
    while (customHeader.hasNext()) {
        customHeader.next();
        auto header = new KMime::Headers::Generic(customHeader.key().constData());
        header->fromUnicodeString(customHeader.value(), "utf-8");
        message->setHeader(header);
    }
    message->assemble();
    connect(qjob, &MailTransport::MessageQueueJob::result, this, &ComposerViewBase::slotQueueResult);
    m_pendingQueueJobs++;
    qjob->start();

    qCDebug(MESSAGECOMPOSER_LOG) << "Queued a message.";
}

void ComposerViewBase::slotQueueResult(KJob *job)
{
    m_pendingQueueJobs--;
    MailTransport::MessageQueueJob *qjob = static_cast<MailTransport::MessageQueueJob * >(job);
    qCDebug(MESSAGECOMPOSER_LOG) << "mPendingQueueJobs" << m_pendingQueueJobs;
    Q_ASSERT(m_pendingQueueJobs >= 0);

    if (job->error()) {
        qCDebug(MESSAGECOMPOSER_LOG) << "Failed to queue a message:" << job->errorString();
        // There is not much we can do now, since all the MessageQueueJobs have been
        // started.  So just wait for them to finish.
        // TODO show a message box or something
        QString msg = i18n("There were problems trying to queue the message for sending: %1",
                           job->errorString());

        if (m_pendingQueueJobs == 0) {
            Q_EMIT failed(msg);
            return;
        }
    }

    if (m_pendingQueueJobs == 0) {
        addFollowupReminder(qjob->message()->messageID(false)->asUnicodeString());
        Q_EMIT sentSuccessfully();
    }
}

void ComposerViewBase::fillQueueJobHeaders(MailTransport::MessageQueueJob *qjob, KMime::Message::Ptr message, const MessageComposer::InfoPart *infoPart)
{
    MailTransport::Transport *transport = MailTransport::TransportManager::self()->transportById(infoPart->transportId());
    if (transport && transport->specifySenderOverwriteAddress()) {
        qjob->addressAttribute().setFrom(KEmailAddress::extractEmailAddress(KEmailAddress::normalizeAddressesAndEncodeIdn(transport->senderOverwriteAddress())));
    } else {
        qjob->addressAttribute().setFrom(KEmailAddress::extractEmailAddress(KEmailAddress::normalizeAddressesAndEncodeIdn(infoPart->from())));
    }
    // if this header is not empty, it contains the real recipient of the message, either the primary or one of the
    //  secondary recipients. so we set that to the transport job, while leaving the message itself alone.
    if (KMime::Headers::Base *realTo = message->headerByType("X-KMail-EncBccRecipients")) {
        qjob->addressAttribute().setTo(cleanEmailList(encodeIdn(realTo->asUnicodeString().split(QLatin1Char('%')))));
        message->removeHeader("X-KMail-EncBccRecipients");
        message->assemble();
        qCDebug(MESSAGECOMPOSER_LOG) << "sending with-bcc encr mail to a/n recipient:" <<  qjob->addressAttribute().to();
    } else {
        qjob->addressAttribute().setTo(cleanEmailList(encodeIdn(infoPart->to())));
        qjob->addressAttribute().setCc(cleanEmailList(encodeIdn(infoPart->cc())));
        qjob->addressAttribute().setBcc(cleanEmailList(encodeIdn(infoPart->bcc())));
    }
}

void ComposerViewBase::initAutoSave()
{
    qCDebug(MESSAGECOMPOSER_LOG) << "initalising autosave";

    // Ensure that the autosave directory exists.
    QDir dataDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kmail2/"));
    if (!dataDirectory.exists(QStringLiteral("autosave"))) {
        qCDebug(MESSAGECOMPOSER_LOG) << "Creating autosave directory.";
        dataDirectory.mkdir(QStringLiteral("autosave"));
    }

    // Construct a file name
    if (m_autoSaveUUID.isEmpty()) {
        m_autoSaveUUID = QUuid::createUuid().toString();
    }

    updateAutoSave();
}
Akonadi::Collection ComposerViewBase::followUpCollection() const
{
    return mFollowUpCollection;
}

void ComposerViewBase::setFollowUpCollection(const Akonadi::Collection &followUpCollection)
{
    mFollowUpCollection = followUpCollection;
}

QDate ComposerViewBase::followUpDate() const
{
    return mFollowUpDate;
}

void ComposerViewBase::setFollowUpDate(const QDate &followUpDate)
{
    mFollowUpDate = followUpDate;
}

Sonnet::DictionaryComboBox *ComposerViewBase::dictionary() const
{
    return m_dictionary;
}

void ComposerViewBase::setDictionary(Sonnet::DictionaryComboBox *dictionary)
{
    m_dictionary = dictionary;
}

void ComposerViewBase::updateAutoSave()
{
    if (m_autoSaveInterval == 0) {
        delete m_autoSaveTimer;
        m_autoSaveTimer = nullptr;
    } else {
        if (!m_autoSaveTimer) {
            m_autoSaveTimer = new QTimer(this);
            if (m_parentWidget) {
                connect(m_autoSaveTimer, SIGNAL(timeout()),
                        m_parentWidget, SLOT(autoSaveMessage()));
            } else {
                connect(m_autoSaveTimer, &QTimer::timeout,
                        this, &ComposerViewBase::autoSaveMessage);
            }
        }
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
}

void ComposerViewBase::cleanupAutoSave()
{
    delete m_autoSaveTimer;
    m_autoSaveTimer = nullptr;
    if (!m_autoSaveUUID.isEmpty()) {

        qCDebug(MESSAGECOMPOSER_LOG) << "deleting autosave files" << m_autoSaveUUID;

        // Delete the autosave files
        QDir autoSaveDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kmail2/autosave"));

        // Filter out only this composer window's autosave files
        QStringList autoSaveFilter;
        autoSaveFilter << m_autoSaveUUID + QLatin1String("*");
        autoSaveDir.setNameFilters(autoSaveFilter);

        // Return the files to be removed
        const QStringList autoSaveFiles = autoSaveDir.entryList();
        qCDebug(MESSAGECOMPOSER_LOG) << "There are" << autoSaveFiles.count() << "to be deleted.";

        // Delete each file
        for (const QString &file : autoSaveFiles) {
            autoSaveDir.remove(file);
        }
        m_autoSaveUUID.clear();
    }
}

//-----------------------------------------------------------------------------
void ComposerViewBase::autoSaveMessage()
{
    qCDebug(MESSAGECOMPOSER_LOG) << "Autosaving message";

    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
    }

    if (!m_composers.isEmpty()) {
        // This may happen if e.g. the autosave timer calls applyChanges.
        qCDebug(MESSAGECOMPOSER_LOG) << "Called while composer active; ignoring.";
        return;
    }

    MessageComposer::Composer *const composer = createSimpleComposer();
    composer->setAutoSave(true);
    m_composers.append(composer);
    connect(composer, &MessageComposer::Composer::result, this, &ComposerViewBase::slotAutoSaveComposeResult);
    composer->start();
}

void ComposerViewBase::setAutoSaveFileName(const QString &fileName)
{
    m_autoSaveUUID = fileName;

    Q_EMIT modified(true);
}

void ComposerViewBase::slotAutoSaveComposeResult(KJob *job)
{
    using MessageComposer::Composer;

    Q_ASSERT(dynamic_cast< Composer * >(job));
    Composer *composer = static_cast< Composer * >(job);

    if (composer->error() == Composer::NoError) {
        Q_ASSERT(m_composers.contains(composer));

        // The messages were composed successfully. Only save the first message, there should
        // only be one anyway, since crypto is disabled.
        qCDebug(MESSAGECOMPOSER_LOG) << "NoError.";
        writeAutoSaveToDisk(composer->resultMessages().first());
        Q_ASSERT(composer->resultMessages().size() == 1);

        if (m_autoSaveInterval > 0) {
            updateAutoSave();
        }
    } else if (composer->error() == MessageComposer::Composer::UserCancelledError) {
        // The job warned the user about something, and the user chose to return
        // to the message.  Nothing to do.
        qCDebug(MESSAGECOMPOSER_LOG) << "UserCancelledError.";
        Q_EMIT failed(i18n("Job cancelled by the user"), AutoSave);
    } else {
        qCDebug(MESSAGECOMPOSER_LOG) << "other Error.";
        Q_EMIT failed(i18n("Could not autosave message: %1", job->errorString()), AutoSave);
    }

    m_composers.removeAll(composer);
}

void ComposerViewBase::writeAutoSaveToDisk(const KMime::Message::Ptr &message)
{
    const QString autosavePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kmail2/autosave/");
    QDir().mkpath(autosavePath);
    const QString filename = autosavePath + m_autoSaveUUID;
    QSaveFile file(filename);
    QString errorMessage;
    qCDebug(MESSAGECOMPOSER_LOG) << "Writing message to disk as" << filename;

    if (file.open(QIODevice::WriteOnly)) {
        file.setPermissions(QFile::ReadUser | QFile::WriteUser);

        if (file.write(message->encodedContent()) !=
                static_cast<qint64>(message->encodedContent().size())) {
            errorMessage = i18n("Could not write all data to file.");
        } else {
            if (!file.commit()) {
                errorMessage = i18n("Could not finalize the file.");
            }
        }
    } else {
        errorMessage = i18n("Could not open file.");
    }

    if (!errorMessage.isEmpty()) {
        qCWarning(MESSAGECOMPOSER_LOG) << "Auto saving failed:" << errorMessage << file.errorString() << " m_autoSaveUUID" << m_autoSaveUUID;
        if (!m_autoSaveErrorShown) {
            KMessageBox::sorry(m_parentWidget, i18n("Autosaving the message as %1 failed.\n"
                                                    "%2\n"
                                                    "Reason: %3",
                                                    filename,
                                                    errorMessage,
                                                    file.errorString()),
                               i18n("Autosaving Message Failed"));

            // Error dialog shown, hide the errors the next time
            m_autoSaveErrorShown = true;
        }
    } else {
        // No error occurred, the next error should be shown again
        m_autoSaveErrorShown = false;
    }
    file.commit();
}

void ComposerViewBase::saveMessage(const KMime::Message::Ptr &message, MessageComposer::MessageSender::SaveIn saveIn)
{
    Akonadi::Collection target;
    const KIdentityManagement::Identity identity = identityManager()->identityForUoid(m_identityCombo->currentIdentity());
    message->date()->setDateTime(QDateTime::currentDateTime());
    message->assemble();

    Akonadi::Item item;
    item.setMimeType(QStringLiteral("message/rfc822"));
    item.setPayload(message);
    Akonadi::MessageFlags::copyMessageFlags(*message, item);

    if (!identity.isNull()) {   // we have a valid identity
        if (saveIn == MessageComposer::MessageSender::SaveInTemplates) {
            if (!identity.templates().isEmpty()) {   // the user has specified a custom templates collection
                target = Akonadi::Collection(identity.templates().toLongLong());
            }
        } else {
            if (!identity.drafts().isEmpty()) {   // the user has specified a custom drafts collection
                target = Akonadi::Collection(identity.drafts().toLongLong());
            }
        }
        Akonadi::CollectionFetchJob *saveMessageJob = new Akonadi::CollectionFetchJob(target, Akonadi::CollectionFetchJob::Base);
        saveMessageJob->setProperty("Akonadi::Item", QVariant::fromValue(item));
        QObject::connect(saveMessageJob, &Akonadi::CollectionFetchJob::result, this, &ComposerViewBase::slotSaveMessage);
    } else {
        // preinitialize with the default collections
        if (saveIn == MessageComposer::MessageSender::SaveInTemplates) {
            target = Akonadi::SpecialMailCollections::self()->defaultCollection(Akonadi::SpecialMailCollections::Templates);
        } else {
            target = Akonadi::SpecialMailCollections::self()->defaultCollection(Akonadi::SpecialMailCollections::Drafts);
        }
        Akonadi::ItemCreateJob *create = new Akonadi::ItemCreateJob(item, target, this);
        connect(create, &Akonadi::ItemCreateJob::result, this, &ComposerViewBase::slotCreateItemResult);
        ++m_pendingQueueJobs;
    }
}

void ComposerViewBase::slotSaveMessage(KJob *job)
{
    Akonadi::Collection target;
    Akonadi::Item item = job->property("Akonadi::Item").value<Akonadi::Item>();
    if (job->error()) {
        target = defaultSpecialTarget();
    } else {
        const Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
        if (fetchJob->collections().isEmpty()) {
            target = defaultSpecialTarget();
        } else {
            target = fetchJob->collections().at(0);
        }
    }
    Akonadi::ItemCreateJob *create = new Akonadi::ItemCreateJob(item, target, this);
    connect(create, &Akonadi::ItemCreateJob::result, this, &ComposerViewBase::slotCreateItemResult);
    ++m_pendingQueueJobs;
}

Akonadi::Collection ComposerViewBase::defaultSpecialTarget() const
{
    Akonadi::Collection target;
    if (mSaveIn == MessageComposer::MessageSender::SaveInTemplates) {
        target = Akonadi::SpecialMailCollections::self()->defaultCollection(Akonadi::SpecialMailCollections::Templates);
    } else {
        target = Akonadi::SpecialMailCollections::self()->defaultCollection(Akonadi::SpecialMailCollections::Drafts);
    }
    return target;
}

void ComposerViewBase::slotCreateItemResult(KJob *job)
{
    --m_pendingQueueJobs;
    qCDebug(MESSAGECOMPOSER_LOG) << "mPendingCreateItemJobs" << m_pendingQueueJobs;
    Q_ASSERT(m_pendingQueueJobs >= 0);

    if (job->error()) {
        qCWarning(MESSAGECOMPOSER_LOG) << "Failed to save a message:" << job->errorString();
        Q_EMIT failed(i18n("Failed to save the message: %1", job->errorString()));
        return;
    }

    if (mSendLaterInfo) {
        Akonadi::ItemCreateJob *createJob = static_cast<Akonadi::ItemCreateJob *>(job);
        const Akonadi::Item item = createJob->item();
        if (item.isValid()) {
            mSendLaterInfo->setItemId(item.id());
            SendLater::SendLaterUtil::writeSendLaterInfo(SendLater::SendLaterUtil::defaultConfig(), mSendLaterInfo);
            delete mSendLaterInfo;
            mSendLaterInfo = nullptr;
        }
    }

    if (m_pendingQueueJobs == 0) {
        Q_EMIT sentSuccessfully();
    }
}

void ComposerViewBase::addAttachment(const QUrl &url, const QString &comment, bool sync)
{
    Q_UNUSED(comment);
    qCDebug(MESSAGECOMPOSER_LOG) << "adding attachment with url:" << url;
    if (sync) {
        m_attachmentController->addAttachmentUrlSync(url);
    } else {
        m_attachmentController->addAttachment(url);
    }
}

void ComposerViewBase::addAttachment(const QString &name, const QString &filename, const QString &charset, const QByteArray &data, const QByteArray &mimeType)
{
    MessageCore::AttachmentPart::Ptr attachment = MessageCore::AttachmentPart::Ptr(new MessageCore::AttachmentPart());
    if (!data.isEmpty()) {
        attachment->setName(name);
        attachment->setFileName(filename);
        attachment->setData(data);
        attachment->setCharset(charset.toLatin1());
        attachment->setMimeType(mimeType);
        // TODO what about the other fields?

        m_attachmentController->addAttachment(attachment);
    }
}

void ComposerViewBase::addAttachmentPart(KMime::Content *partToAttach)
{
    MessageCore::AttachmentPart::Ptr part(new MessageCore::AttachmentPart);
    if (partToAttach->contentType()->mimeType() == "multipart/digest" ||
            partToAttach->contentType()->mimeType() == "message/rfc822") {
        // if it is a digest or a full message, use the encodedContent() of the attachment,
        // which already has the proper headers
        part->setData(partToAttach->encodedContent());
    } else {
        part->setData(partToAttach->decodedContent());
    }
    part->setMimeType(partToAttach->contentType()->mimeType());
    if (partToAttach->contentDescription(false)) {
        part->setDescription(partToAttach->contentDescription()->asUnicodeString());
    }
    if (partToAttach->contentType(false)) {
        if (partToAttach->contentType()->hasParameter(QStringLiteral("name"))) {
            part->setName(partToAttach->contentType()->parameter(QStringLiteral("name")));
        }
    }
    if (partToAttach->contentDisposition(false)) {
        part->setFileName(partToAttach->contentDisposition()->filename());
        part->setInline(partToAttach->contentDisposition()->disposition() == KMime::Headers::CDinline);
    }
    if (part->name().isEmpty() && !part->fileName().isEmpty()) {
        part->setName(part->fileName());
    }
    if (part->fileName().isEmpty() && !part->name().isEmpty()) {
        part->setFileName(part->name());
    }
    m_attachmentController->addAttachment(part);
}

MessageComposer::Composer *ComposerViewBase::createSimpleComposer()
{
    MessageComposer::Composer *composer = new MessageComposer::Composer;
    fillGlobalPart(composer->globalPart());
    m_editor->fillComposerTextPart(composer->textPart());
    fillInfoPart(composer->infoPart(), UseUnExpandedRecipients);
    if (m_attachmentModel)  {
        composer->addAttachmentParts(m_attachmentModel->attachments());
    }
    return composer;
}

//-----------------------------------------------------------------------------
QString ComposerViewBase::to() const
{
    return MessageComposer::Util::cleanedUpHeaderString(m_recipientsEditor->recipientString(MessageComposer::Recipient::To));
}

//-----------------------------------------------------------------------------
QString ComposerViewBase::cc() const
{
    return MessageComposer::Util::cleanedUpHeaderString(m_recipientsEditor->recipientString(MessageComposer::Recipient::Cc));
}

//-----------------------------------------------------------------------------
QString ComposerViewBase::bcc() const
{
    return MessageComposer::Util::cleanedUpHeaderString(m_recipientsEditor->recipientString(MessageComposer::Recipient::Bcc));
}

QString ComposerViewBase::from() const
{
    return MessageComposer::Util::cleanedUpHeaderString(m_from);
}

QString ComposerViewBase::replyTo() const
{
    return MessageComposer::Util::cleanedUpHeaderString(m_replyTo);
}

QString ComposerViewBase::subject() const
{
    return MessageComposer::Util::cleanedUpHeaderString(m_subject);
}

void ComposerViewBase::setParentWidgetForGui(QWidget *w)
{
    m_parentWidget = w;
}

void ComposerViewBase::setAttachmentController(MessageComposer::AttachmentControllerBase *controller)
{
    m_attachmentController = controller;
}

MessageComposer::AttachmentControllerBase *ComposerViewBase::attachmentController()
{
    return m_attachmentController;
}

void ComposerViewBase::setAttachmentModel(MessageComposer::AttachmentModel *model)
{
    m_attachmentModel = model;
}

MessageComposer::AttachmentModel *ComposerViewBase::attachmentModel()
{
    return m_attachmentModel;
}

void ComposerViewBase::setRecipientsEditor(MessageComposer::RecipientsEditor *recEditor)
{
    m_recipientsEditor = recEditor;
}

MessageComposer::RecipientsEditor *ComposerViewBase::recipientsEditor()
{
    return m_recipientsEditor;
}

void ComposerViewBase::setSignatureController(MessageComposer::SignatureController *sigController)
{
    m_signatureController = sigController;
}

MessageComposer::SignatureController *ComposerViewBase::signatureController()
{
    return m_signatureController;
}

void ComposerViewBase::setIdentityCombo(KIdentityManagement::IdentityCombo *identCombo)
{
    m_identityCombo = identCombo;
}

KIdentityManagement::IdentityCombo *ComposerViewBase::identityCombo()
{
    return m_identityCombo;
}

void ComposerViewBase::updateRecipients(const KIdentityManagement::Identity &ident, const KIdentityManagement::Identity &oldIdent, MessageComposer::Recipient::Type type)
{
    QString oldIdentList;
    QString newIdentList;
    if (type == MessageComposer::Recipient::Bcc) {
        oldIdentList = oldIdent.bcc();
        newIdentList = ident.bcc();

    } else if (type == MessageComposer::Recipient::Cc) {
        oldIdentList = oldIdent.cc();
        newIdentList = ident.cc();
    } else {
        return;
    }

    if (oldIdentList != newIdentList) {
        const auto oldRecipients = KMime::Types::Mailbox::listFromUnicodeString(oldIdentList);
        for (const KMime::Types::Mailbox &recipient : oldRecipients) {
            m_recipientsEditor->removeRecipient(recipient.prettyAddress(), type);
        }

        const auto newRecipients = KMime::Types::Mailbox::listFromUnicodeString(newIdentList);
        for (const KMime::Types::Mailbox &recipient : newRecipients) {
            m_recipientsEditor->addRecipient(recipient.prettyAddress(), type);
        }
        m_recipientsEditor->setFocusBottom();
    }
}

void ComposerViewBase::identityChanged(const KIdentityManagement::Identity &ident, const KIdentityManagement::Identity &oldIdent, bool msgCleared)
{
    updateRecipients(ident, oldIdent, MessageComposer::Recipient::Bcc);
    updateRecipients(ident, oldIdent, MessageComposer::Recipient::Cc);

    KIdentityManagement::Signature oldSig = const_cast<KIdentityManagement::Identity &>
                                            (oldIdent).signature();
    KIdentityManagement::Signature newSig = const_cast<KIdentityManagement::Identity &>
                                            (ident).signature();
    //replace existing signatures
    const bool replaced = editor()->composerSignature()->replaceSignature(oldSig, newSig);
    // Just append the signature if there was no old signature
    if (!replaced && (msgCleared || oldSig.rawText().isEmpty())) {
        signatureController()->applySignature(newSig);
    }
    const QString vcardFileName = ident.vCardFile();
    attachmentController()->setIdentityHasOwnVcard(!vcardFileName.isEmpty());
    attachmentController()->setAttachOwnVcard(ident.attachVcard());

    m_editor->setAutocorrectionLanguage(ident.autocorrectionLanguage());

}

void ComposerViewBase::setEditor(MessageComposer::RichTextComposerNg *editor)
{
    m_editor = editor;
    m_editor->document()->setModified(false);
}

MessageComposer::RichTextComposerNg *ComposerViewBase::editor() const
{
    return m_editor;
}

void ComposerViewBase::setTransportCombo(MailTransport::TransportComboBox *transpCombo)
{
    m_transport = transpCombo;
}

MailTransport::TransportComboBox *ComposerViewBase::transportComboBox() const
{
    return m_transport;
}

void ComposerViewBase::setIdentityManager(KIdentityManagement::IdentityManager *identMan)
{
    m_identMan = identMan;
}

KIdentityManagement::IdentityManager *ComposerViewBase::identityManager()
{
    return m_identMan;
}

void ComposerViewBase::setFcc(const Akonadi::Collection &fccCollection)
{
    if (m_fccCombo) {
        m_fccCombo->setDefaultCollection(fccCollection);
    } else {
        m_fccCollection = fccCollection;
    }
    Akonadi::CollectionFetchJob *const checkFccCollectionJob =
        new Akonadi::CollectionFetchJob(fccCollection, Akonadi::CollectionFetchJob::Base);
    connect(checkFccCollectionJob, &KJob::result, this, &ComposerViewBase::slotFccCollectionCheckResult);
}

void ComposerViewBase::slotFccCollectionCheckResult(KJob *job)
{
    if (job->error()) {
        const Akonadi::Collection sentMailCol =
            Akonadi::SpecialMailCollections::self()->defaultCollection(Akonadi::SpecialMailCollections::SentMail);
        if (m_fccCombo) {
            m_fccCombo->setDefaultCollection(sentMailCol);
        } else {
            m_fccCollection = sentMailCol;
        }
    }
}

void ComposerViewBase::setFccCombo(Akonadi::CollectionComboBox *fcc)
{
    m_fccCombo = fcc;
}

Akonadi::CollectionComboBox *ComposerViewBase::fccCombo() const
{
    return m_fccCombo;
}

void ComposerViewBase::setFrom(const QString &from)
{
    m_from = from;
}

void ComposerViewBase::setReplyTo(const QString &replyTo)
{
    m_replyTo = replyTo;
}

void ComposerViewBase::setSubject(const QString &subject)
{
    m_subject = subject;
    if (mSendLaterInfo) {
        mSendLaterInfo->setSubject(m_subject);
        mSendLaterInfo->setTo(to());
    }
}

void ComposerViewBase::setAutoSaveInterval(int interval)
{
    m_autoSaveInterval = interval;
}

void ComposerViewBase::setCryptoOptions(bool sign, bool encrypt, Kleo::CryptoMessageFormat format, bool neverEncryptDrafts)
{
    m_sign = sign;
    m_encrypt = encrypt;
    m_cryptoMessageFormat = format;
    m_neverEncrypt = neverEncryptDrafts;
}

void ComposerViewBase::setCharsets(const QList< QByteArray > &charsets)
{
    m_charsets = charsets;
}

void ComposerViewBase::setMDNRequested(bool mdnRequested)
{
    m_mdnRequested = mdnRequested;
}

void ComposerViewBase::setUrgent(bool urgent)
{
    m_urgent = urgent;
}

QStringList ComposerViewBase::cleanEmailList(const QStringList &emails)
{
    QStringList clean;
    clean.reserve(emails.count());
    for (const QString &email : emails) {
        clean << KEmailAddress::extractEmailAddress(email);
    }
    return clean;
}

int ComposerViewBase::autoSaveInterval() const
{
    return m_autoSaveInterval;
}

//-----------------------------------------------------------------------------
void ComposerViewBase::collectImages(KMime::Content *root)
{
    if (KMime::Content *n = Util::findTypeInMessage(root, "multipart", "alternative")) {
        KMime::Content *parentnode = n->parent();
        if (parentnode &&
                parentnode->contentType()->isMultipart() &&
                parentnode->contentType()->subType() == "related") {
            KMime::Content *node = MessageCore::NodeHelper::nextSibling(n);
            while (node) {
                if (node->contentType()->isImage()) {
                    qCDebug(MESSAGECOMPOSER_LOG) << "found image in multipart/related : " << node->contentType()->name();
                    QImage img;
                    img.loadFromData(node->decodedContent());
                    m_editor->composerControler()->composerImages()->loadImage(img, QString::fromLatin1(QByteArray(QByteArrayLiteral("cid:") + node->contentID()->identifier())),
                            node->contentType()->name());
                }
                node = MessageCore::NodeHelper::nextSibling(node);
            }
        }
    }
}

//-----------------------------------------------------------------------------
bool ComposerViewBase::inlineSigningEncryptionSelected()
{
    if (!m_sign && !m_encrypt) {
        return false;
    }
    return m_cryptoMessageFormat == Kleo::InlineOpenPGPFormat;
}

bool ComposerViewBase::hasMissingAttachments(const QStringList &attachmentKeywords)
{
    if (attachmentKeywords.isEmpty()) {
        return false;
    }
    if (m_attachmentModel && m_attachmentModel->rowCount() > 0) {
        return false;
    }

    QStringList attachWordsList = attachmentKeywords;

    QRegExp rx(QLatin1String("\\b") +
               attachWordsList.join(QStringLiteral("\\b|\\b")) +
               QLatin1String("\\b"));
    rx.setCaseSensitivity(Qt::CaseInsensitive);

    // check whether the subject contains one of the attachment key words
    // unless the message is a reply or a forwarded message
    const QString subj = subject();
    bool gotMatch = (MessageHelper::stripOffPrefixes(subj) == subj) && (rx.indexIn(subj) >= 0);

    if (!gotMatch) {
        // check whether the non-quoted text contains one of the attachment key
        // words
        QRegExp quotationRx(QStringLiteral("^([ \\t]*([|>:}#]|[A-Za-z]+>))+"));
        QTextDocument *doc = m_editor->document();
        QTextBlock end(doc->end());
        for (QTextBlock it = doc->begin(); it != end; it = it.next()) {
            const QString line = it.text();
            gotMatch = (quotationRx.indexIn(line) < 0) &&
                       (rx.indexIn(line) >= 0);
            if (gotMatch) {
                break;
            }
        }
    }

    if (!gotMatch) {
        return false;
    }
    return true;
}

ComposerViewBase::MissingAttachment ComposerViewBase::checkForMissingAttachments(const QStringList &attachmentKeywords)
{
    if (!hasMissingAttachments(attachmentKeywords)) {
        return NoMissingAttachmentFound;
    }
    int rc = KMessageBox::warningYesNoCancel(m_editor,
             i18n("The message you have composed seems to refer to an "
                  "attached file but you have not attached anything.\n"
                  "Do you want to attach a file to your message?"),
             i18n("File Attachment Reminder"),
             KGuiItem(i18n("&Attach File...")),
             KGuiItem(i18n("&Send as Is")));
    if (rc == KMessageBox::Cancel) {
        return FoundMissingAttachmentAndCancel;
    }
    if (rc == KMessageBox::Yes) {
        m_attachmentController->showAddAttachmentFileDialog();
        return FoundMissingAttachmentAndAddedAttachment;
    }

    return FoundMissingAttachmentAndSending;
}

void ComposerViewBase::markAllAttachmentsForSigning(bool sign)
{
    if (m_attachmentModel) {
        foreach (MessageCore::AttachmentPart::Ptr attachment, m_attachmentModel->attachments()) {
            attachment->setSigned(sign);
        }
    }
}

void ComposerViewBase::markAllAttachmentsForEncryption(bool encrypt)
{
    if (m_attachmentModel) {
        foreach (MessageCore::AttachmentPart::Ptr attachment, m_attachmentModel->attachments()) {
            attachment->setEncrypted(encrypt);
        }
    }
}

bool ComposerViewBase::determineWhetherToSign(bool doSignCompletely, Kleo::KeyResolver *keyResolver, bool signSomething, bool &result, bool &canceled)
{
    bool sign = false;
    switch (keyResolver->checkSigningPreferences(signSomething)) {
    case Kleo::DoIt:
        if (!signSomething) {
            markAllAttachmentsForSigning(true);
            return true;
        }
        sign = true;
        break;
    case Kleo::DontDoIt:
        sign = false;
        break;
    case Kleo::AskOpportunistic:
        assert(0);
    case Kleo::Ask: {
        // the user wants to be asked or has to be asked
#ifndef QT_NO_CURSOR
        KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
        const QString msg = i18n("Examination of the recipient's signing preferences "
                                 "yielded that you be asked whether or not to sign "
                                 "this message.\n"
                                 "Sign this message?");
        switch (KMessageBox::questionYesNoCancel(m_parentWidget, msg,
                i18n("Sign Message?"),
                KGuiItem(i18nc("to sign", "&Sign")),
                KGuiItem(i18n("Do &Not Sign")))) {
        case KMessageBox::Cancel:
            result = false;
            canceled = true;
            return false;
        case KMessageBox::Yes:
            markAllAttachmentsForSigning(true);
            return true;
        case KMessageBox::No:
            markAllAttachmentsForSigning(false);
            return false;
        default:
            qCWarning(MESSAGECOMPOSER_LOG) << "Unhandled MessageBox response";
            return false;
        }
    }
    break;
    case Kleo::Conflict: {
        // warn the user that there are conflicting signing preferences
#ifndef QT_NO_CURSOR
        KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
        const QString msg = i18n("There are conflicting signing preferences "
                                 "for these recipients.\n"
                                 "Sign this message?");
        switch (KMessageBox::warningYesNoCancel(m_parentWidget, msg,
                                                i18n("Sign Message?"),
                                                KGuiItem(i18nc("to sign", "&Sign")),
                                                KGuiItem(i18n("Do &Not Sign")))) {
        case KMessageBox::Cancel:
            result = false;
            canceled = true;
            return false;
        case KMessageBox::Yes:
            markAllAttachmentsForSigning(true);
            return true;
        case KMessageBox::No:
            markAllAttachmentsForSigning(false);
            return false;
        default:
            qCWarning(MESSAGECOMPOSER_LOG) << "Unhandled MessageBox response";
            return false;
        }
    }
    break;
    case Kleo::Impossible: {
#ifndef QT_NO_CURSOR
        KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
        const QString msg = i18n("You have requested to sign this message, "
                                 "but no valid signing keys have been configured "
                                 "for this identity.");
        if (KMessageBox::warningContinueCancel(m_parentWidget, msg,
                                               i18n("Send Unsigned?"),
                                               KGuiItem(i18n("Send &Unsigned")))
                == KMessageBox::Cancel) {
            result = false;
            return false;
        } else {
            markAllAttachmentsForSigning(false);
            return false;
        }
    }
    }

    if (!sign || !doSignCompletely) {
        if (MessageComposer::MessageComposerSettings::self()->cryptoWarningUnsigned()) {
#ifndef QT_NO_CURSOR
            KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
            const QString msg = sign && !doSignCompletely ?
                                i18n("Some parts of this message will not be signed.\n"
                                     "Sending only partially signed messages might violate site policy.\n"
                                     "Sign all parts instead?") // oh, I hate this...
                                : i18n("This message will not be signed.\n"
                                       "Sending unsigned message might violate site policy.\n"
                                       "Sign message instead?"); // oh, I hate this...
            const QString buttonText = sign && !doSignCompletely
                                       ? i18n("&Sign All Parts") : i18n("&Sign");
            switch (KMessageBox::warningYesNoCancel(m_parentWidget, msg,
                                                    i18n("Unsigned-Message Warning"),
                                                    KGuiItem(buttonText),
                                                    KGuiItem(i18n("Send &As Is")))) {
            case KMessageBox::Cancel:
                result = false;
                canceled = true;
                return false;
            case KMessageBox::Yes:
                markAllAttachmentsForSigning(true);
                return true;
            case KMessageBox::No:
                return sign || doSignCompletely;
            default:
                qCWarning(MESSAGECOMPOSER_LOG) << "Unhandled MessageBox response";
                return false;
            }
        }
    }
    return sign || doSignCompletely;
}

bool ComposerViewBase::determineWhetherToEncrypt(bool doEncryptCompletely, Kleo::KeyResolver *keyResolver, bool encryptSomething, bool signSomething, bool &result, bool &canceled)
{
    bool encrypt = false;
    bool opportunistic = false;
    switch (keyResolver->checkEncryptionPreferences(encryptSomething)) {
    case Kleo::DoIt:
        if (!encryptSomething) {
            markAllAttachmentsForEncryption(true);
            return true;
        }
        encrypt = true;
        break;
    case Kleo::DontDoIt:
        encrypt = false;
        break;
    case Kleo::AskOpportunistic:
        opportunistic = true;
    // fall through...
#if QT_VERSION >= QT_VERSION_CHECK(5,8,0)
        Q_FALLTHROUGH();
#endif
    case Kleo::Ask: {
        // the user wants to be asked or has to be asked
#ifndef QT_NO_CURSOR
        KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
        const QString msg = opportunistic
                            ? i18n("Valid trusted encryption keys were found for all recipients.\n"
                                   "Encrypt this message?")
                            : i18n("Examination of the recipient's encryption preferences "
                                   "yielded that you be asked whether or not to encrypt "
                                   "this message.\n"
                                   "Encrypt this message?");
        switch (KMessageBox::questionYesNoCancel(m_parentWidget, msg,
                i18n("Encrypt Message?"),
                KGuiItem(signSomething
                         ? i18n("Sign && &Encrypt")
                         : i18n("&Encrypt")),
                KGuiItem(signSomething
                         ? i18n("&Sign Only")
                         : i18n("&Send As-Is")))) {
        case KMessageBox::Cancel:
            result = false;
            canceled = true;
            return false;
        case KMessageBox::Yes:
            markAllAttachmentsForEncryption(true);
            return true;
        case KMessageBox::No:
            markAllAttachmentsForEncryption(false);
            return false;
        default:
            qCWarning(MESSAGECOMPOSER_LOG) << "Unhandled MessageBox response";
            return false;
        }
    }
    break;
    case Kleo::Conflict: {
        // warn the user that there are conflicting encryption preferences
#ifndef QT_NO_CURSOR
        KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
        const QString msg = i18n("There are conflicting encryption preferences "
                                 "for these recipients.\n"
                                 "Encrypt this message?");
        switch (KMessageBox::warningYesNoCancel(m_parentWidget, msg,
                                                i18n("Encrypt Message?"),
                                                KGuiItem(i18n("&Encrypt")),
                                                KGuiItem(i18n("Do &Not Encrypt")))) {
        case KMessageBox::Cancel:
            result = false;
            canceled = true;
            return false;
        case KMessageBox::Yes:
            markAllAttachmentsForEncryption(true);
            return true;
        case KMessageBox::No:
            markAllAttachmentsForEncryption(false);
            return false;
        default:
            qCWarning(MESSAGECOMPOSER_LOG) << "Unhandled MessageBox response";
            return false;
        }
    }
    break;
    case Kleo::Impossible: {
#ifndef QT_NO_CURSOR
        KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
        const QString msg = i18n("You have requested to encrypt this message, "
                                 "and to encrypt a copy to yourself, "
                                 "but no valid trusted encryption keys have been "
                                 "configured for this identity.");
        if (KMessageBox::warningContinueCancel(m_parentWidget, msg,
                                               i18n("Send Unencrypted?"),
                                               KGuiItem(i18n("Send &Unencrypted")))
                == KMessageBox::Cancel) {
            result = false;
            return false;
        } else {
            markAllAttachmentsForEncryption(false);
            return false;
        }
    }
    }

    if (!encrypt || !doEncryptCompletely) {
        if (MessageComposer::MessageComposerSettings::self()->cryptoWarningUnencrypted()) {
#ifndef QT_NO_CURSOR
            KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
            const QString msg = !doEncryptCompletely ?
                                i18n("Some parts of this message will not be encrypted.\n"
                                     "Sending only partially encrypted messages might violate "
                                     "site policy and/or leak sensitive information.\n"
                                     "Encrypt all parts instead?") // oh, I hate this...
                                : i18n("This message will not be encrypted.\n"
                                       "Sending unencrypted messages might violate site policy and/or "
                                       "leak sensitive information.\n"
                                       "Encrypt messages instead?"); // oh, I hate this...
            const QString buttonText = !doEncryptCompletely
                                       ? i18n("&Encrypt All Parts") : i18n("&Encrypt");
            switch (KMessageBox::warningYesNoCancel(m_parentWidget, msg,
                                                    i18n("Unencrypted Message Warning"),
                                                    KGuiItem(buttonText),
                                                    KGuiItem(signSomething
                                                            ? i18n("&Sign Only")
                                                            : i18n("&Send As-Is")))) {
            case KMessageBox::Cancel:
                result = false;
                canceled = true;
                return false;
            case KMessageBox::Yes:
                markAllAttachmentsForEncryption(true);
                return true;
            case KMessageBox::No:
                return encrypt || doEncryptCompletely;
            default:
                qCWarning(MESSAGECOMPOSER_LOG) << "Unhandled MessageBox response";
                return false;
            }
        }
    }

    return encrypt || doEncryptCompletely;
}

void ComposerViewBase::setSendLaterInfo(SendLater::SendLaterInfo *info)
{
    delete mSendLaterInfo;
    mSendLaterInfo = info;
}

SendLater::SendLaterInfo *ComposerViewBase::sendLaterInfo() const
{
    return mSendLaterInfo;
}

void ComposerViewBase::addFollowupReminder(const QString &messageId)
{
    if (!messageId.isEmpty()) {
        if (mFollowUpDate.isValid()) {
            MessageComposer::FollowupReminderCreateJob *job = new MessageComposer::FollowupReminderCreateJob;
            job->setSubject(m_subject);
            job->setMessageId(messageId);
            job->setTo(m_replyTo);
            job->setFollowUpReminderDate(mFollowUpDate);
            job->setCollectionToDo(mFollowUpCollection);
            job->start();
        }
    }
}

