#include "findsupport.h"

FindSupport::FindSupport()
    : m_text(NULL),
      m_rememberText(false)
{
}

void FindSupport::cancel()
{
    if (m_text && !m_rememberText) {
        delete m_text;
        m_text = NULL;
    }
}

void FindSupport::setText(QString text)
{
    if (m_text) {
        delete m_text;
    }
    m_text = new QString(text);
}

QString FindSupport::text()
{
    return m_text ? QString(*m_text) : "";
}

bool FindSupport::find(QString text)
{
    setText(text);
    find();
}


FindSupport::~FindSupport()
{
    if (m_text) {
        delete m_text;
        m_text = NULL;
    }
}

