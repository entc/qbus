#ifndef __QBUS__HPP__H
#define __QBUS__HPP__H 1

// STL includes
#include <stdexcept>

// cape includes
#include <hpp/cape_stc.hpp>

// qbus include
#include <qbus.h>
#include <stdexcept>

namespace qbus
{

  //-----------------------------------------------------------------------------
  
  class Message
  {
    
  public:
    
    Message (QBus qbus, QBusM qin, QBusM qout) : m_qbus (qbus), m_qin (qin), m_qout (qout), m_cdata_in (qin->cdata), m_ret (CAPE_ERR_NONE) {}
    
    void set_continue (cape::Udc& content)
    {
      m_ret = CAPE_ERR_CONTINUE;
      
      qbus_message_clr (m_qin, CAPE_UDC_UNDEFINED);
      
      if (content.valid())
      {
        m_qin->cdata = content.release();
      }
    }
    
    cape::Udc& cdata () { return m_cdata_in; }
    
    cape::Udc& cdata_valid (int type)
    {
      if (m_cdata_in.empty())
      {
        throw std::runtime_error ("no 'cdata'");
      }
      
      if (m_cdata_in.type() != type)
      {
        throw std::runtime_error ("cdata' is not a node");
      }
      
      return m_cdata_in;
    }
    
    cape::Udc output (int type)
    {
      if (m_qout == NULL)
      {
        throw std::runtime_error ("output can't be set");
      }
      
      m_qout->cdata = cape_udc_new (type, NULL);
      
      return cape::Udc (m_qout->cdata);
    }
    
    void forward ()
    {
      cape_udc_replace_mv (&(m_qout->cdata), &(m_qin->cdata));
    }
    
    QBus qbus () { return m_qbus; }
    
    QBusM qin () { return m_qin; }
    
    int ret () { return m_ret; }
    
  private:
    
    QBus m_qbus;
    
    QBusM m_qin;
    
    QBusM m_qout;
    
    cape::Udc m_cdata_in;
    
    int m_ret;
    
  };
  
  
}

#endif
