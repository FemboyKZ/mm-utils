#ifndef _INCLUDE_MMU_HTTP_CLIENT_H_
#define _INCLUDE_MMU_HTTP_CLIENT_H_

// Minimal async HTTP(S) GET/POST client.
// Windows: WinHTTP. Linux: libcurl.
// Requests are queued and dispatched on a single background worker thread.

#include <functional>
#include <string>

namespace mmu
{
	namespace http
	{
		// Callback type: (success, responseBody)
		// WARNING: callbacks are invoked on a background thread. Do NOT touch game state directly from them.
		// Use QueueMainThread to schedule any work that needs to run on the game thread.
		using Callback = std::function<void(bool success, std::string body)>;

		// User agent sent with every request. Set once at plugin load.
		void SetUserAgent(const char *userAgent);

		// GET request. url must be https:// or http://.
		void Get(const std::string &url, Callback callback);

		// POST request with JSON body.
		void Post(const std::string &url, const std::string &jsonBody, Callback callback);

		// POST request with application/x-www-form-urlencoded body.
		void PostForm(const std::string &url, const std::string &formBody, Callback callback);

		// Cancel any in-flight request and join the worker thread. Call on plugin unload.
		void Shutdown();

		// Re-arm after Shutdown so the worker can restart on the next request.
		// Call from plugin Load so an unload/reload cycle leaves HTTP functional.
		void ResetShutdownLatch();

		// Schedule a function to run on the game thread during the next GameFrame.
		// Thread-safe: safe to call from a Callback.
		void QueueMainThread(std::function<void()> fn);

		// Drain the main-thread queue. Call from the GameFrame hook, game thread only.
		void DrainMainThread();

		// Discard any queued main-thread callbacks without executing them.
		// Call at the very end of plugin Unload after hooks are gone.
		void ClearMainQueue();
	} // namespace http
} // namespace mmu

#endif // _INCLUDE_MMU_HTTP_CLIENT_H_
