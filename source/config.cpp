#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <regex>
#include <stdlib.h>
#include "config.h"
#include "fs.h"
#include "lang.h"

extern "C"
{
#include "inifile.h"
}

bool swap_xo;
RemoteSettings *remote_settings;
char local_directory[255];
char remote_directory[255];
char app_ver[6];
char last_site[32];
char display_site[32];
char language[128];
std::vector<std::string> sites;
std::vector<std::string> http_servers;
std::map<std::string, RemoteSettings> site_settings;
PackageUrlInfo install_pkg_url;
char favorite_urls[MAX_FAVORITE_URLS][512];
bool auto_delete_tmp_pkg;
RemoteClient *remoteclient;

namespace CONFIG
{

    void SetClientType(RemoteSettings *setting)
    {
        if (strncmp(setting->server, "smb://", 6) == 0)
        {
            setting->type = CLIENT_TYPE_SMB;
        }
        else if (strncmp(setting->server, "ftp://", 6) == 0)
        {
            setting->type = CLIENT_TYPE_FTP;
        }
        else if (strncmp(setting->server, "webdav://", 9) == 0 || strncmp(setting->server, "webdavs://", 10) == 0)
        {
            setting->type = CLIENT_TYPE_WEBDAV;
        }
        else if (strncmp(setting->server, "http://", 7) == 0 || strncmp(setting->server, "https://", 8) == 0)
        {
            setting->type = CLIENT_TYPE_HTTP_SERVER;
        }
        else
        {
            setting->type = CLINET_TYPE_UNKNOWN;
        }
    }

    void LoadConfig()
    {
        if (!FS::FolderExists(DATA_PATH))
        {
            FS::MkDirs(DATA_PATH);
        }

        sites = {"Site 1", "Site 2", "Site 3", "Site 4", "Site 5", "Site 6", "Site 7", "Site 8", "Site 9", "Site 10",
                 "Site 11", "Site 12", "Site 13", "Site 14", "Site 15", "Site 16", "Site 17", "Site 18", "Site 19", "Site 20"};

        http_servers = { HTTP_SERVER_APACHE, HTTP_SERVER_MS_IIS, HTTP_SERVER_NGINX, HTTP_SERVER_NPX_SERVE};

        OpenIniFile(CONFIG_INI_FILE);

        int version = ReadInt(CONFIG_GLOBAL, CONFIG_VERSION, 0);
        bool conversion_needed = false;
        if (version < CONFIG_VERSION_NUM)
        {
            conversion_needed = true;
        }
        WriteInt(CONFIG_GLOBAL, CONFIG_VERSION, CONFIG_VERSION_NUM);

        // Load global config
        sprintf(language, "%s", ReadString(CONFIG_GLOBAL, CONFIG_LANGUAGE, ""));
        WriteString(CONFIG_GLOBAL, CONFIG_LANGUAGE, language);

        sprintf(local_directory, "%s", ReadString(CONFIG_GLOBAL, CONFIG_LOCAL_DIRECTORY, "/"));
        WriteString(CONFIG_GLOBAL, CONFIG_LOCAL_DIRECTORY, local_directory);

        sprintf(remote_directory, "%s", ReadString(CONFIG_GLOBAL, CONFIG_REMOTE_DIRECTORY, "/"));
        WriteString(CONFIG_GLOBAL, CONFIG_REMOTE_DIRECTORY, remote_directory);

        auto_delete_tmp_pkg = ReadBool(CONFIG_GLOBAL, CONFIG_AUTO_DELETE_TMP_PKG, true);
        WriteBool(CONFIG_GLOBAL, CONFIG_AUTO_DELETE_TMP_PKG, auto_delete_tmp_pkg);

        for (int i = 0; i < sites.size(); i++)
        {
            RemoteSettings setting;
            sprintf(setting.site_name, "%s", sites[i].c_str());

            sprintf(setting.server, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_SERVER_URL, ""));
            if (conversion_needed && strlen(setting.server)>0)
            {
                std::string tmp = std::string(setting.server);
                tmp = std::regex_replace(tmp, std::regex("http://"), "webdav://");
                tmp = std::regex_replace(tmp, std::regex("https://"), "webdavs://");
                sprintf(setting.server, "%s", tmp.c_str());
            }
            WriteString(sites[i].c_str(), CONFIG_REMOTE_SERVER_URL, setting.server);

            sprintf(setting.username, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_SERVER_USER, ""));
            WriteString(sites[i].c_str(), CONFIG_REMOTE_SERVER_USER, setting.username);

            sprintf(setting.password, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_SERVER_PASSWORD, ""));
            WriteString(sites[i].c_str(), CONFIG_REMOTE_SERVER_PASSWORD, setting.password);

            setting.http_port = ReadInt(sites[i].c_str(), CONFIG_REMOTE_SERVER_HTTP_PORT, 80);
            WriteInt(sites[i].c_str(), CONFIG_REMOTE_SERVER_HTTP_PORT, setting.http_port);

            setting.enable_rpi = ReadBool(sites[i].c_str(), CONFIG_ENABLE_RPI, false);
            WriteBool(sites[i].c_str(), CONFIG_ENABLE_RPI, setting.enable_rpi);

            sprintf(setting.http_server_type, "%s", ReadString(sites[i].c_str(), CONFIG_REMOTE_HTTP_SERVER_TYPE, HTTP_SERVER_APACHE));
            WriteString(sites[i].c_str(), CONFIG_REMOTE_HTTP_SERVER_TYPE, setting.http_server_type);

            SetClientType(&setting);
            site_settings.insert(std::make_pair(sites[i], setting));
        }

        sprintf(last_site, "%s", ReadString(CONFIG_GLOBAL, CONFIG_LAST_SITE, sites[0].c_str()));
        WriteString(CONFIG_GLOBAL, CONFIG_LAST_SITE, last_site);

        remote_settings = &site_settings[std::string(last_site)];

        for (int i = 0; i < MAX_FAVORITE_URLS; i++)
        {
            const char *index = std::to_string(i).c_str();
            sprintf(favorite_urls[i], "%s", ReadString(CONFIG_FAVORITE_URLS, index, ""));
            WriteString(CONFIG_FAVORITE_URLS, index, favorite_urls[i]);
        }

        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void SaveConfig()
    {
        OpenIniFile(CONFIG_INI_FILE);

        WriteString(last_site, CONFIG_REMOTE_SERVER_URL, remote_settings->server);
        WriteString(last_site, CONFIG_REMOTE_SERVER_USER, remote_settings->username);
        WriteString(last_site, CONFIG_REMOTE_SERVER_PASSWORD, remote_settings->password);
        WriteInt(last_site, CONFIG_REMOTE_SERVER_HTTP_PORT, remote_settings->http_port);
        WriteBool(last_site, CONFIG_ENABLE_RPI, remote_settings->enable_rpi);
        WriteString(last_site, CONFIG_REMOTE_HTTP_SERVER_TYPE, remote_settings->http_server_type);
        WriteString(CONFIG_GLOBAL, CONFIG_LAST_SITE, last_site);
        WriteBool(CONFIG_GLOBAL, CONFIG_AUTO_DELETE_TMP_PKG, auto_delete_tmp_pkg);
        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void SaveFavoriteUrl(int index, char *url)
    {
        OpenIniFile(CONFIG_INI_FILE);
        const char *idx = std::to_string(index).c_str();
        WriteString(CONFIG_FAVORITE_URLS, idx, url);
        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void ParseMultiValueString(const char *prefix_list, std::vector<std::string> &prefixes, bool toLower)
    {
        std::string prefix = "";
        int length = strlen(prefix_list);
        for (int i = 0; i < length; i++)
        {
            char c = prefix_list[i];
            if (c != ' ' && c != '\t' && c != ',')
            {
                if (toLower)
                {
                    prefix += std::tolower(c);
                }
                else
                {
                    prefix += c;
                }
            }

            if (c == ',' || i == length - 1)
            {
                prefixes.push_back(prefix);
                prefix = "";
            }
        }
    }

    std::string GetMultiValueString(std::vector<std::string> &multi_values)
    {
        std::string vts = std::string("");
        if (multi_values.size() > 0)
        {
            for (int i = 0; i < multi_values.size() - 1; i++)
            {
                vts.append(multi_values[i]).append(",");
            }
            vts.append(multi_values[multi_values.size() - 1]);
        }
        return vts;
    }

    void RemoveFromMultiValues(std::vector<std::string> &multi_values, std::string value)
    {
        auto itr = std::find(multi_values.begin(), multi_values.end(), value);
        if (itr != multi_values.end())
            multi_values.erase(itr);
    }
}
