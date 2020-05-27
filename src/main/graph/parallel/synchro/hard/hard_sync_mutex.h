#ifndef HARD_SYNC_MUTEX_H
#define HARD_SYNC_MUTEX_H

#include <queue>
#include "api/communication/communication.h"
#include "api/graph/parallel/location_state.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"
#include "mutex_client.h"
#include "mutex_server.h"

namespace FPMAS::graph::parallel::synchro::hard {

	using FPMAS::api::graph::parallel::LocationState;
	using FPMAS::api::graph::parallel::synchro::hard::MutexRequestType;

	template<typename T>
		class HardSyncMutex
			: public FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T> {
			private:
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexRequest
					Request;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexClient<T>
					MutexClient;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T>
					MutexServer;

				DistributedId id;
				std::reference_wrapper<T> _data;
				LocationState* state;
				int* location;
				bool _locked = false;
				int _locked_shared = 0;

				MutexClient* mutex_client;
				MutexServer* mutex_server;

				//std::queue<Request> readRequests;
				//std::queue<Request> lockRequests;
				//std::queue<Request> acquireRequests;

				std::queue<Request> lock_requests;
				std::queue<Request> lock_shared_requests;

				void _lock() override {_locked=true;}
				void _lockShared() override {_locked_shared++;}
				void _unlock() override {_locked=false;}
				void _unlockShared() override {_locked_shared--;}
			public:
				HardSyncMutex(T& data) : _data(std::ref(data)) {}

				void setUp(
					DistributedId id,
					LocationState& state,
					int& location,
					MutexClient& mutex_client,
					MutexServer& mutex_server) {
					this->id = id;
					this->state = &state;
					this->location = &location;
					this->mutex_client = &mutex_client;
					this->mutex_server = &mutex_server;
				}

				void pushRequest(Request request) override;
				std::queue<Request> requestsToProcess() override;

				T& data() override {return _data;}

				const T& read() override;
				void releaseRead() override;

				T& acquire() override;
				void releaseAcquire() override;

				void lock() override;
				void unlock() override;
				bool locked() const override {return _locked;}

				void lockShared() override;
				void unlockShared() override;
				int lockedShared() const override {return _locked_shared;}
		};

	template<typename T>
		const T& HardSyncMutex<T>::read() {
			if(*state == LocationState::LOCAL) {
				if(_locked) {
					Request req = Request(id, Request::LOCAL, MutexRequestType::READ);
					pushRequest(req);
					mutex_server->wait(req);
				}
				_locked_shared++;
				return _data;
			}
			_data.get() = mutex_client->read(id, *location);
			return _data;
		}
	template<typename T>
		void HardSyncMutex<T>::releaseRead() {
			if(*state == LocationState::LOCAL) {
				_locked_shared--;
				if(_locked_shared==0) {
					mutex_server->notify(id);
				}
				return;
			}
			mutex_client->releaseRead(id, *location);
		}

	template<typename T>
		T& HardSyncMutex<T>::acquire() {
			if(*state==LocationState::LOCAL) {
				if(_locked || _locked_shared > 0) {
					Request req = Request(id, Request::LOCAL, MutexRequestType::ACQUIRE);
					pushRequest(req);
					mutex_server->wait(req);
				}
				this->_locked = true;
				return _data;
			}
			_data.get() = mutex_client->acquire(id, *location);
			return _data;
		}

	template<typename T>
		void HardSyncMutex<T>::releaseAcquire() {
			if(*state==LocationState::LOCAL) {
				mutex_server->notify(id);
				this->_locked = false;
				return;
			}
			mutex_client->releaseAcquire(id, _data, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::lock() {
			if(*state==LocationState::LOCAL) {
				if(_locked || _locked_shared > 0) {
					Request req = Request(id, Request::LOCAL, MutexRequestType::LOCK);
					pushRequest(req);
					mutex_server->wait(req);
				}
				this->_locked = true;
				return;
			}
			mutex_client->lock(id, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::unlock() {
			if(*state==LocationState::LOCAL) {
				// TODO : this is NOT thread safe
				this->_locked = false;
				mutex_server->notify(id);
				return;
			}
			mutex_client->unlock(id, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::lockShared() {
			if(*state==LocationState::LOCAL) {
				if(_locked) {
					Request req = Request(id, Request::LOCAL, MutexRequestType::LOCK_SHARED);
					pushRequest(req);
					mutex_server->wait(req);
				}
				_locked_shared++;
				return;
			}
			mutex_client->lockShared(id, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::unlockShared() {
			if(*state==LocationState::LOCAL) {
				_locked_shared--;
				if(_locked_shared==0) {
					mutex_server->notify(id);
				}
				return;
			}
			mutex_client->unlockShared(id, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::pushRequest(Request request) {
			switch(request.type) {
				case MutexRequestType::READ :
					lock_shared_requests.push(request);
					break;
				case MutexRequestType::LOCK :
					lock_requests.push(request);
					break;
				case MutexRequestType::ACQUIRE :
					lock_requests.push(request);
					break;
				case MutexRequestType::LOCK_SHARED:
					lock_shared_requests.push(request);
			}
		}

	// TODO : this must be the job of a ReadersWriters component
	template<typename T>
		std::queue<typename HardSyncMutex<T>::Request> HardSyncMutex<T>::requestsToProcess() {
			std::queue<Request> requests;
			if(_locked) {
				return requests;
			}
			while(!lock_shared_requests.empty()) {
				Request lock_shared_request = lock_shared_requests.front();
				requests.push(lock_shared_request);
				lock_shared_requests.pop();
				if(lock_shared_request.source == Request::LOCAL) {
					// Immediately returns, so that the last request processed
					// is the local request
					return requests;
				}
			}
			if(!lock_requests.empty()) {
				requests.push(lock_requests.front());
				lock_requests.pop();
			}
			return requests;
		}
}

#endif
