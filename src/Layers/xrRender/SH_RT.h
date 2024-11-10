#pragma once

//////////////////////////////////////////////////////////////////////////
class		CRT		:	public xr_resource_named	{
public:
	CRT();
	~CRT();
	void	create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false );
	void	destroy();
	void	reset_begin();
	void	reset_end();
	IC BOOL	valid()	{ return !!pTexture; }

public:
	ID3DTexture2D*			pSurface;
	ID3DRenderTargetView*	pRT;
	ID3DDepthStencilView*	pZRT;
	ID3D11UnorderedAccessView*	pUAView;

	ref_texture				pTexture;

	u32						dwWidth;
	u32						dwHeight;
	D3DFORMAT				fmt;

	u64						_order;
};
struct 		resptrcode_crt	: public resptr_base<CRT>
{
	void				create			(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false );
	void				destroy			()	{ _set(NULL);		}
};
typedef	resptr_core<CRT,resptrcode_crt>		ref_rt;
