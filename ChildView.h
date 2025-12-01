// ChildView.h
#pragma once

#include <vector>
#include <atlimage.h>   // CImage

// ----------------------
// 노드 / 엣지 정보 구조체
// ----------------------
struct NODE_INFO
{
    CPoint pt;       // 화면 좌표
    bool   selected; // 선택/강조 여부

    NODE_INFO(CPoint p = CPoint(0, 0))
        : pt(p), selected(false) {}
};

struct EDGE_INFO
{
    int    u;         // 시작 노드 인덱스
    int    v;         // 끝 노드 인덱스
    double weight;    // 거리(가중치)
    bool   isShortest;// 최단 경로에 포함 여부

    EDGE_INFO(int _u = -1, int _v = -1, double w = 0.0)
        : u(_u), v(_v), weight(w), isShortest(false) {}
};


// CChildView 창

class CChildView : public CWnd
{
    // 생성
public:
    CChildView();

    // 특성
public:

    // 작업
public:

    // 재정의
protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    // 구현
public:
    virtual ~CChildView();

protected:
    // ===== 지도 비트맵 =====
    CImage m_map;        // 배경 지도 이미지
    BOOL   m_bMapLoaded; // 로딩 성공 여부

    // ===== 그래프 관련 멤버 변수 =====
    std::vector<NODE_INFO> m_nodes;  // 노드들
    std::vector<EDGE_INFO> m_edges;  // 엣지들

    int m_edgeStartIndex;   // Alt+클릭으로 엣지 만들 때 첫 번째 노드
    int m_pathStartIndex;   // Ctrl+Shift+클릭 시작 노드
    int m_pathEndIndex;     // Ctrl+Shift+클릭 끝 노드

    // ===== 내부 함수 =====
    int  HitTestNode(CPoint pt);          // 클릭 위치에 노드가 있는지 검사
    void DrawGraph(CDC* pDC);             // 노드/엣지 그리기
    void RunDijkstra(int start, int goal);// 최단 경로 계산(Dijkstra)

    // 생성된 메시지 맵 함수
protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()
};
