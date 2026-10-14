// Copyright © 2026 CCP ehf.

#include "LabPage.h"

namespace AudioLab
{
	std::unique_ptr<LabPage> CreateSandboxPage();
	std::unique_ptr<LabPage> CreateOcclusionPage();
	std::unique_ptr<LabPage> CreatePortalsPage();

	std::vector<std::unique_ptr<LabPage>> CreatePages()
	{
		std::vector<std::unique_ptr<LabPage>> pages;
		pages.push_back( CreateSandboxPage() );
		pages.push_back( CreateOcclusionPage() );
		pages.push_back( CreatePortalsPage() );
		return pages;
	}
}
