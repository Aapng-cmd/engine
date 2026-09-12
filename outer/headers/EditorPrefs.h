#pragma once

#include <QHash>
#include <QString>
#include <QVariant>

class QComboBox;
class QFormLayout;
class QLabel;
class QObject;
class QWidget;

enum class EditorLanguage { English, Russian };
enum class EditorTheme { Dark, Light };

class EditorPrefs {
public:
    static EditorPrefs& instance();

    void load();
    void save() const;

    EditorLanguage language() const { return m_language; }
    void setLanguage(EditorLanguage lang);

    EditorTheme theme() const { return m_theme; }
    void setTheme(EditorTheme theme);

    QString languageCode() const;
    static EditorLanguage languageFromCode(const QString& code);
    QString themeCode() const;
    static EditorTheme themeFromCode(const QString& code);

private:
    EditorLanguage m_language = EditorLanguage::English;
    EditorTheme m_theme = EditorTheme::Dark;
};

QString i18n(const char* en);
QString i18n(const QString& en);

void applyEditorTheme(EditorTheme theme);

void markI18n(QObject* o, const char* key);
void retranslateI18nProperties(QWidget* root);
QLabel* makeI18nLabel(const char* key, QWidget* parent = nullptr);
void addI18nFormRow(QFormLayout* form, const char* key, QWidget* field);
void addI18nComboItem(QComboBox* c, const char* key, const QVariant& data);
void retranslateComboI18n(QComboBox* c);
void markI18nTip(QWidget* w, const char* key);
void markI18nPlaceholder(QWidget* w, const char* key);
