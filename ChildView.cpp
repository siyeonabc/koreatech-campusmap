#include "pch.h"
#include "framework.h"
#include "ChildView.h"

#include <math.h>      
#include <wchar.h>   
#include <Shlwapi.h>   
#pragma comment(lib, "Shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



CChildView::CChildView()
{
    m_edgeStartIndex = -1;
    m_pathStartIndex = -1;
    m_pathEndIndex = -1;

    // ===== 지도 이미지 로드 =====
    m_bMapLoaded = FALSE;

    // 실행 파일이 있는 폴더 경로 구하기
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);  
    PathRemoveFileSpecW(exePath);                     


    wcscat_s(exePath, L"\\map.png");

    HRESULT hr = m_map.Load(exePath);
    if (SUCCEEDED(hr))
    {
        m_bMapLoaded = TRUE;
    }
    else
    {
        AfxMessageBox(L"map.png 로드 실패\nexe 폴더에 map.png 가 있는지 확인하세요.");
    }
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()



BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CWnd::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        nullptr);

    return TRUE;
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}


int CChildView::HitTestNode(CPoint pt)
{
    const int r = 8; 

    // 뒤에서부터 확인하면 마지막에 만든 노드를 우선 선택
    for (int i = (int)m_nodes.size() - 1; i >= 0; --i)
    {
        CRect rc(m_nodes[i].pt.x - r, m_nodes[i].pt.y - r,
            m_nodes[i].pt.x + r, m_nodes[i].pt.y + r);
        if (rc.PtInRect(pt))
            return i;
    }
    return -1;
}


void CChildView::DrawGraph(CDC* pDC)
{
    for (const auto& e : m_edges)
    {
        if (e.u < 0 || e.v < 0 ||
            e.u >= (int)m_nodes.size() ||
            e.v >= (int)m_nodes.size())
            continue;

        CPoint p1 = m_nodes[e.u].pt;
        CPoint p2 = m_nodes[e.v].pt;


        CPen pen(e.isShortest ? PS_SOLID : PS_SOLID,
            e.isShortest ? 6 : 1,              
            e.isShortest ? RGB(255, 0, 0) : RGB(0, 0, 255));
        CPen* pOldPen = pDC->SelectObject(&pen);

        pDC->MoveTo(p1);
        pDC->LineTo(p2);

        pDC->SelectObject(pOldPen);


    }

    // ----- 노드(동그라미) 그림 -----
    const int r = 6;

    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        const NODE_INFO& n = m_nodes[i];

        COLORREF color = n.selected ? RGB(0, 0, 255) : RGB(0, 128, 255);
        CBrush brush(color);
        CBrush* pOldBrush = pDC->SelectObject(&brush);

        pDC->Ellipse(n.pt.x - r, n.pt.y - r,
            n.pt.x + r, n.pt.y + r);

        pDC->SelectObject(pOldBrush);
    }
}


void CChildView::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);


    if (m_bMapLoaded)
    {
        m_map.Draw(dc.m_hDC,
            rcClient.left, rcClient.top,
            rcClient.Width(), rcClient.Height(),
            0, 0,
            m_map.GetWidth(), m_map.GetHeight());
    }
    else
    {
        dc.FillSolidRect(&rcClient, RGB(255, 255, 255));
    }

    DrawGraph(&dc);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    BOOL bCtrl = (nFlags & MK_CONTROL) != 0;
    BOOL bShift = (nFlags & MK_SHIFT) != 0;
    BOOL bAlt = (GetKeyState(VK_MENU) & 0x8000) != 0; // Alt 키

    // 1) Ctrl + 클릭 : 노드 생성
    if (bCtrl && !bAlt && !bShift)
    {
        NODE_INFO node(point);
        m_nodes.push_back(node);
    }
    // 2) Alt + 클릭 : 두 노드를 선택해서 엣지 생성
    else if (bAlt && !bCtrl && !bShift)
    {
        int idx = HitTestNode(point);
        if (idx != -1)
        {
            if (m_edgeStartIndex == -1)
            {
                // 첫 번째 노드 선택
                m_edgeStartIndex = idx;
                m_nodes[idx].selected = true;
            }
            else if (m_edgeStartIndex != idx)
            {
                // 두 번째 노드 선택 -> 엣지 생성
                int u = m_edgeStartIndex;
                int v = idx;

                int dx = m_nodes[u].pt.x - m_nodes[v].pt.x;
                int dy = m_nodes[u].pt.y - m_nodes[v].pt.y;
                double dist = sqrt((double)dx * dx + (double)dy * dy);

                EDGE_INFO edge(u, v, dist);
                edge.isShortest = false;
                m_edges.push_back(edge);

                m_nodes[v].selected = true;

                m_edgeStartIndex = -1; // 다시 초기화
            }
        }
    }
    // 3) Shift + 클릭 : 두 노드 선택하고 최단 경로 표시
    else if (bShift && !bCtrl && !bAlt)
    {
        int idx = HitTestNode(point);
        if (idx != -1)
        {
            if (m_pathStartIndex == -1)
            {
                m_pathStartIndex = idx;
            }
            else if (m_pathEndIndex == -1 && idx != m_pathStartIndex)
            {
                m_pathEndIndex = idx;

                // Dijkstra 실행 
                RunDijkstra(m_pathStartIndex, m_pathEndIndex);

                // 다음 선택을 위해 초기화
                m_pathStartIndex = -1;
                m_pathEndIndex = -1;
            }
        }
    }

    Invalidate(); 
    CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::RunDijkstra(int start, int goal)
{
    int n = (int)m_nodes.size();
    if (start < 0 || goal < 0 || start >= n || goal >= n)
        return;

    const double INF = 1e18;

    std::vector<double> dist(n, INF);
    std::vector<int>    prev(n, -1);
    std::vector<bool>   used(n, false);

    dist[start] = 0.0;

    for (auto& e : m_edges)
        e.isShortest = false;

    // ----- Dijkstra 메인 루프 -----
    for (int i = 0; i < n; ++i)
    {
        int u = -1;
        double best = INF;

   
        for (int v = 0; v < n; ++v)
        {
            if (!used[v] && dist[v] < best)
            {
                best = dist[v];
                u = v;
            }
        }

        if (u == -1)
            break; 

        used[u] = true;


        for (const auto& e : m_edges)
        {
            int v = -1;
            double w = e.weight;

            if (e.u == u)
                v = e.v;
            else if (e.v == u)
                v = e.u;
            else
                v = -1;

            if (v == -1)
                continue;

            if (dist[u] + w < dist[v])
            {
                dist[v] = dist[u] + w;
                prev[v] = u;
            }
        }
    }

    if (dist[goal] == INF)
    {
        AfxMessageBox(L"두 점 사이에 연결된 경로가 없습니다.");
        return;
    }

    int cur = goal;
    while (prev[cur] != -1)
    {
        int p = prev[cur];

        for (auto& e : m_edges)
        {
            if ((e.u == p && e.v == cur) ||
                (e.u == cur && e.v == p))
            {
                e.isShortest = true;
                break;
            }
        }

        cur = p;
    }

    CString msg;
    msg.Format(L"선택한 두 점 사이의 최단 거리 = %.0f", dist[goal]);
    AfxMessageBox(msg);
}
