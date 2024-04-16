#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QSharedPointer>

class Cue : public QWidget
{
    Q_OBJECT
public:
    explicit Cue(const QString cue_sound_file = "./notify.wav", QWidget* parent = nullptr);
    ~Cue();

#if 0
    void set_audio_complete() { m_audio_complete = true; }
#endif

signals:

public slots:
    void    slot_trigger_audio();
    void    slot_trigger_visual(const QString display_text);

private slots:
#if 0
    void    slot_check_completion();
#endif

private:
    bool    m_audio_available{false};       // is the audio cue playable?
#if 0
    bool    m_audio_complete{false};        // has the audio cue finished playing?
    bool    m_visual_complete{false};       // has the visual cue finsihed playing?
#endif

    QLabel* m_textlabel{nullptr};

    QRect   m_initial_r, m_target_r;
};

using CuePointer = QSharedPointer<Cue>;
