#include "githubhelper.h"

string GetLatestUpdaterVersion() {
	CComPtr<IStream> pStream;

	HRESULT hr = URLOpenBlockingStreamW(nullptr, L"https://api.github.com/repos/ckosmic/gm_retromod_updater/tags?per_page=1", &pStream, 0, nullptr);
	if (FAILED(hr)) {
		return NULL;
	}

	char buffer[8192];
	do {
		DWORD bytesRead = 0;
		hr = pStream->Read(buffer, sizeof(buffer), &bytesRead);
		buffer[bytesRead] = 0;
		if (bytesRead > 0) {
			json j = json::parse(string(buffer));
			string out;
			j.at(0).at("name").get_to(out);
			return out;
		}
	} while (SUCCEEDED(hr) && hr != S_FALSE);

	if (FAILED(hr)) {
		return NULL;
	}
}

bool DownloadUpdater(string version) {
	CComPtr<IStream> pStream;

	string dlUrl = "https://api.github.com/repos/ckosmic/gm_retromod_updater/releases/tags/" + version;
	HRESULT hr = URLOpenBlockingStream(nullptr, dlUrl.c_str(), &pStream, 0, nullptr);
	if (FAILED(hr)) {
		return false;
	}

	char buffer[8192];
	do {
		DWORD bytesRead = 0;
		hr = pStream->Read(buffer, sizeof(buffer), &bytesRead);
		buffer[bytesRead] = 0;
		if (bytesRead > 0) {
			json j = json::parse(string(buffer));
			vector<string> urls(2);
			vector<string> names(2);
			j.at("assets").at(0).at("browser_download_url").get_to(urls[0]);
			j.at("assets").at(1).at("browser_download_url").get_to(urls[1]);
			j.at("assets").at(0).at("name").get_to(names[0]);
			j.at("assets").at(1).at("name").get_to(names[1]);

			for (int i = 0; i < urls.size(); i++) {
				string url = urls[i];
				string dest = "garrysmod\\lua\\bin\\tmp_" + names[i];
				string target = "garrysmod\\lua\\bin\\" + names[i];
				HRESULT result = URLDownloadToFile(NULL, url.c_str(), dest.c_str(), 0, NULL);

				if (!SUCCEEDED(result)) {
					return false;
				}
				remove(target.c_str());
				rename(dest.c_str(), target.c_str());
			}
			return true;
		}
		return false;
	} while (SUCCEEDED(hr) && hr != S_FALSE);

	if (FAILED(hr)) {
		return false;
	}

	return true;
}