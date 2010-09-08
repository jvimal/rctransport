
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include <string>
using namespace std;


namespace ns3 {

  class RCClientHelper {
  public:
    RCClientHelper() {
      m_factory.SetTypeId("ns3::RCClient");      
    }

    void SetAttribute(string name, const AttributeValue &value) {
      m_factory.Set(name, value);
    }

    ApplicationContainer Install(Ptr<Node> node) {
      Ptr<Application> app = m_factory.Create<Application>();
      node->AddApplication(app);
      return ApplicationContainer(app);
    }

    ApplicationContainer Install(NodeContainer nc) {
      ApplicationContainer ret;
      for(uint32_t i=0;i < nc.GetN(); i++) {
        ret.Add(Install(nc.Get(i)));
      }
      return ret;
    }

  private:
    ObjectFactory m_factory;
  };

  /* repeat the same for server */

  class RCServerHelper {
  public:
    RCServerHelper() {
      m_factory.SetTypeId("ns3::RCServer");
    }

    void SetAttribute(string name, const AttributeValue &value) {
      m_factory.Set(name, value);
    }

    ApplicationContainer Install(Ptr<Node> node) {
      Ptr<Application> app = m_factory.Create<Application>();
      node->AddApplication(app);
      return ApplicationContainer(app);
    }

    ApplicationContainer Install(NodeContainer nc) {
      ApplicationContainer ret;
      for(uint32_t i=0;i < nc.GetN(); i++) {
        ret.Add(Install(nc.Get(i)));
      }
      return ret;
    }
  private:
    ObjectFactory m_factory;
  };

};
