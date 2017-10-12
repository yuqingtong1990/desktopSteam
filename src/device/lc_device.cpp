#include "lc_device.h"

#include <MMSystem.h>  
#include <mmdeviceapi.h>  
#include <endpointvolume.h> 
#include <Functiondiscoverykeys_devpkey.h> 

#include <atlbase.h>
#include <atlstr.h>

#define EXIT_ON_ERROR(hres)  if (FAILED(hres)) { break; }  
#define SAFE_RELEASE(punk)   if ((punk) != NULL)  { (punk)->Release(); (punk) = NULL; }


static std::vector<HMONITOR> g_hMonitorGroup;
int CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lpRMonitor, LPARAM dwData)
{
	g_hMonitorGroup.push_back(hMonitor);
	return TRUE;
}

void Monitor_GetAllInfo(VEC_MONITORMODE_INFO& vecMonitorListInfo)
{
	g_hMonitorGroup.clear();
	::EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

	for (unsigned int i = 0; i < g_hMonitorGroup.size(); i++)
	{
		MONITORINFOEX mixTemp;
		memset(&mixTemp, 0, sizeof(MONITORINFOEX));
		mixTemp.cbSize = sizeof(MONITORINFOEX);

		GetMonitorInfo(g_hMonitorGroup[i], &mixTemp);
		uMonitorInfo mInfo;
		mInfo.wsName = mixTemp.szDevice;
		//判断是否是主要显示器
		mInfo.bMainDisPlay = (mixTemp.dwFlags == MONITORINFOF_PRIMARY);
		mInfo.rcMonitor = mixTemp.rcMonitor;
		mInfo.rcWork = mixTemp.rcWork;
		vecMonitorListInfo.push_back(mInfo);
	}
}

void Device_GetAllInfo(VEC_DEVICE_INFO& vecDeviceListInfo, int type)
{
    vecDeviceListInfo.clear();
    LPWSTR pwszID = NULL;
    IPropertyStore *pProps = NULL;
    HRESULT hr = S_OK;
	do 
	{
		CComPtr<IMMDeviceEnumerator> enumerator;
		hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL);
		EXIT_ON_ERROR(hr)

		EDataFlow dataFlow;
		if (type == 0)
			dataFlow = eCapture;
		else
			dataFlow = eRender;

		CComPtr<IMMDeviceCollection> pDevCollection;
        hr = enumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &pDevCollection);
		EXIT_ON_ERROR(hr)

		UINT dev_count;
		pDevCollection->GetCount(&dev_count);
		EXIT_ON_ERROR(hr)
		
        for (unsigned int i = 0; i < dev_count; i++)
        {
            CComPtr<IMMDevice> pDev;
            hr = pDevCollection->Item(i, &pDev);
            EXIT_ON_ERROR(hr)
            uDeviceInfo devinfo;
            pDev->GetId(&pwszID);
            hr = pDev->OpenPropertyStore(
                STGM_READ, &pProps);
            EXIT_ON_ERROR(hr)
                

            PROPVARIANT varName; 
            PropVariantInit(&varName);
            hr = pProps->GetValue(
                PKEY_Device_FriendlyName, &varName);
            EXIT_ON_ERROR(hr)
            
            devinfo.devid = pwszID;
            devinfo.devName = varName.pwszVal;

            CoTaskMemFree(pwszID);
            pwszID = NULL;
            PropVariantClear(&varName);
            SAFE_RELEASE(pProps)
            vecDeviceListInfo.push_back(devinfo);
        }

	} while (0);

    CoTaskMemFree(pwszID);
    SAFE_RELEASE(pProps);
}
