#include "ns3/queue-disc.h"
#include "ns3/object-factory.h"
#include <list>
#include <map>

namespace ns3 {

	class SfqQueueFlow : public QueueDiscClass {
	
	public:

	private:
		
	}

	class SfqQueueDisc : public QueueDisc {
	public:
	
		/*
			Get the type ID		
		*/
		static TypeId GetTypeId (void);

		/*
			SfqQueueDisc Constructor
		*/	
		SfqQueueDisc ();

		virtual ~SfqQueueDisc ();

		/*
			Set the quantum Value.
		*/		
		/*
			quantum: The number of bytes each queue gets to dequeue on each round of the scheduling algorithm
		*/
		void SetQuantum (uint32_t quantum);
		
		/*
			getter function (returns quantum value)
		*/
		uint32_t GetQuantum (void) const;
		
	private:
		virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
		virtual Ptr<QueueDiscItem> DoDequeue (void);
		virtual void InitializeParams (void);


	}




}



