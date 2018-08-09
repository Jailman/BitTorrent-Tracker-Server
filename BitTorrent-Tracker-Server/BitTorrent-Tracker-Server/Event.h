#pragma once
#include <vector>
#include <Functional>
namespace Event
{
	
	template<typename ArgType>
	class Event
	{
	public:
		using Processer = std::function<void(const ArgType&)>;
	private:
		std::vector<Processer> Processors;
	public:
		Event() {};
        Event(Event&& REvent) 
        {
            this->Processors = std::move(REvent.Processors);
        }
		void Active(const ArgType& Sender)
		{
			for (const auto& ProcessFunc : this->Processors)
				ProcessFunc(Sender);
		};
		~Event() 
		{
			if (!this->Processors.empty()) 
			{
				std::vector<Processer>().swap(this->Processors);
			}
		}
		void operator+=(const Processer& func)
		{
			this->Processors.push_back(func);
		};
		void operator=(Event&& REvent)
        {
            if(!this->Processors.empty())
				std::vector<Processer>().swap(this->Processors);
			this->Processors = std::move(REvent.Processors);
        }
		void Swap(Event& rEvent)
		{
			rEvent.Processors.swap(this->Processors);
		}
	};

}