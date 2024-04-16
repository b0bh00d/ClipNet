#include <QTimer>
#include <QLabel>
#include <QScreen>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QPropertyAnimation>

#include "Cue.h"

#if defined(QT_LINUX)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static ma_engine mini_engine;
static ma_sound cue_sound;
#endif

Cue::Cue(const QString cue_sound_file, QWidget* parent) : QWidget{parent}
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    // setAttribute(Qt::WA_TranslucentBackground);
    setWindowOpacity(0.65);

    // initialize audio playback
#if defined(QT_LINUX)
    (void)ma_engine_init(nullptr, &mini_engine);

    auto result = ma_sound_init_from_file(&mini_engine,
                                          cue_sound_file.toLatin1().constData(),
                                          MA_SOUND_FLAG_DECODE,
                                          nullptr, nullptr, &cue_sound);
    m_audio_available = (result == MA_SUCCESS);
#endif

    // initialize visual formatting

    m_textlabel = new QLabel();
    m_textlabel->setWordWrap(false);
    auto f = font();
    f.setPointSize(24);
    m_textlabel->setFont(f);
    m_textlabel->setStyleSheet("QLabel { color : rgb(150, 150, 0); }");

    auto hlayout = new QHBoxLayout(this);
    hlayout->setMargin(5);
    hlayout->addStretch();
    hlayout->addWidget(m_textlabel);
    hlayout->addStretch();

    setLayout(hlayout);

    auto screen = QGuiApplication::primaryScreen();
    auto geom = screen->availableGeometry();

    auto left = (geom.width() - 500) / 2;
    auto top = (geom.height() - 75) / 2;
    setGeometry(left, top, 500, 75);
}

Cue::~Cue()
{
    ma_sound_uninit(&cue_sound);
    ma_engine_uninit(&mini_engine);
}

#if defined(QT_LINUX)
#if 0
static void my_end_callback(void* pUserData, ma_sound* pSound)
{
    Cue* cue = reinterpret_cast<Cue*>(pUserData);
    if(cue)
        cue->set_audio_complete();
}
#endif
#endif

void Cue::slot_trigger_audio()
{
#if defined(QT_LINUX)
    // On Linux (particularly when interacting with Google Docs in a browser),
    // clipboard notifications are spotty for some reason.  I've added a sound
    // now when the clipboard data is actually sent, as an indicator so I can
    // know the data is actually on the clipnet (and can stop mashing keys).
    //
    // A visual indicator might be better (the speakers my not be on or high
    // enough), but I'll try this for now.
    // https://duckduckgo.com/?t=ffab&q=Qt+5+display+window+with+rounded+corners+and+opacity&ia=web
    // https://stackoverflow.com/questions/1909092/qt4-transparent-window-with-rounded-corners
    // https://doc.qt.io/qt-5/qtwidgets-widgets-shapedclock-example.html
    // https://doc.qt.io/qt-5/qregion.html
    // https://duckduckgo.com/?t=ffab&q=Qt+5+round+window+corners+using+clipmask&ia=web
    // https://stackoverflow.com/questions/25480788/qt-creating-smooth-rounded-corners-of-a-widget
    // https://stackoverflow.com/questions/966688/show-window-in-qt-without-stealing-focus

    // MA_API ma_result ma_sound_set_end_callback(ma_sound* pSound, ma_sound_end_proc callback, void* pUserData);
    // ma_sound_set_end_callback(&cue_sound, my_end_callback, reinterpret_cast<void*>(this));

    // m_audio_complete = false;
    ma_sound_start(&cue_sound);

    // const QString s = QStringLiteral("./notify.wav");
    // ma_engine_play_sound(&mini_engine, s.toLatin1().constData(), nullptr);

    // QTimer::singleShot(300, this, &Cue::slot_check_completion);
#endif
}

const float fWidgetWidth = 0.1304;
const float fWidgetHeight = 0.035;

void Cue::slot_trigger_visual(const QString display_text)
{
    m_textlabel->setText(QString());

    // https://doc.qt.io/qt-5/qparallelanimationgroup.html#details

    auto screen = QGuiApplication::primaryScreen();
    auto geom = screen->availableGeometry();
    auto actual_width = geom.left() + geom.width();
    auto actual_height = geom.top() + geom.height();
    auto widget_width = (int)(fWidgetWidth * actual_width);
    auto widget_height = (int)(fWidgetHeight * actual_height);

    // target position
    auto x = (actual_width - widget_width) / 2;
    auto y = (actual_height - widget_height) / 2;
    m_target_r = QRect(x, y, widget_width, widget_height);

    // initial position
    m_initial_r = QRect(actual_width / 2, y, 0, widget_height);
    setGeometry(m_initial_r);

    QPropertyAnimation* animation = new QPropertyAnimation(this, "geometry");
    animation->setStartValue(m_initial_r);
    animation->setEndValue(m_target_r);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setDuration(50);

    connect(animation, &QPropertyAnimation::finished, this, [this, display_text](){
        if(display_text.length() > 20)
            m_textlabel->setText(QString("%1...").arg(display_text.left(17)));
        else
            m_textlabel->setText(display_text);

        QTimer::singleShot(1000, this, [this]() {
            m_textlabel->setText(QString());

            QPropertyAnimation* animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(m_target_r);
            animation->setEndValue(m_initial_r);
            animation->setEasingCurve(QEasingCurve::OutQuad);
            animation->setDuration(50);

            connect(animation, &QPropertyAnimation::finished, this, [this](){
                hide();
            });

            animation->start();
        });
    });

    animation->start();
    show();

#if 0
    QPropertyAnimation* animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->setDuration(1000);

    connect(animation, &QPropertyAnimation::finished, [this](){
        this->hide();
    });

    animation->start();
    show();

#endif
}

#if 0
void Cue::slot_check_completion()
{
    bool audio_done = (!m_audio_cue) || (m_audio_complete);
    bool visual_done = (!m_visual_cue) || (m_visual_complete);

    if(!audio_done || !visual_done)
        // loop back and check later
        QTimer::singleShot(300, this, &Cue::slot_check_completion);
    else
        // we're all done
        this->deleteLater();
}
#endif
