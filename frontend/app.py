import streamlit as st
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from datetime import datetime, timedelta
from supabase import create_client
import os
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

# Constants for bowl composition
RICE_PROPORTION = 0.50  # 50%
MEAT_PROPORTION = 0.25  # 25%
VEG_PROPORTION = 0.25   # 25%

# Updated color scheme with swapped light/dark colors
COLORS = {
    'rice': {
        'light': '#219ebc',    # Medium blue (now for waste)
        'dark': '#8ecae6'      # Soft blue (now for consumed)
    },
    'meat': {
        'light': '#f4845f',    # Darker coral (now for waste)
        'dark': '#ffb5a7'      # Soft coral (now for consumed)
    },
    'veg': {
        'light': '#40916c',    # Forest green (now for waste)
        'dark': '#95d5b2'      # Soft green (now for consumed)
    }
}

# Initialize Supabase client
@st.cache_resource
def init_supabase():
    supabase_url = os.getenv("SUPABASE_URL", "https://lmrywiafagkxmzpxhjmi.supabase.co")
    supabase_key = os.getenv("SUPABASE_API_KEY", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imxtcnl3aWFmYWdreG16cHhoam1pIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Mjg3MzUwNDYsImV4cCI6MjA0NDMxMTA0Nn0.dDVOxqVHdzjGWwFitOOVEMgmJ-fH6mF4JuPd8goZ8cM")
    return create_client(supabase_url, supabase_key)

# Load and process data from Supabase
@st.cache_data(ttl=300)  # Cache for 5 minutes
def load_data():
    supabase = init_supabase()
    
    # Fetch data from Supabase
    response = supabase.table('food_wastage').select('*').execute()
    
    # Convert to DataFrame
    df = pd.DataFrame(response.data)
    
    # Convert date string to datetime
    df['date'] = pd.to_datetime(df['date'])
    
    # Count bowls prepared by counting entries per stall per date
    bowls_count = df.groupby(['stall', 'date']).size().reset_index(name='bowls_prepared')
    
    # Calculate averages for waste percentages by stall and date
    waste_stats = df.groupby(['stall', 'date']).agg({
        'bowl_weight_kg': 'first',  # Assuming same weight for all bowls
        'rice_waste_percent': 'mean',
        'meat_waste_percent': 'mean',
        'veg_waste_percent': 'mean',
        'cost_per_kg': 'first'  # Assuming same cost for all entries
    }).reset_index()
    
    # Merge the counts with the averages
    df_processed = pd.merge(bowls_count, waste_stats, on=['stall', 'date'])
    
    # Calculate prepared quantities in kg based on bowl proportions
    df_processed['rice_prepared_kg'] = df_processed['bowls_prepared'] * df_processed['bowl_weight_kg'] * RICE_PROPORTION
    df_processed['meat_prepared_kg'] = df_processed['bowls_prepared'] * df_processed['bowl_weight_kg'] * MEAT_PROPORTION
    df_processed['veg_prepared_kg'] = df_processed['bowls_prepared'] * df_processed['bowl_weight_kg'] * VEG_PROPORTION
    
    # Calculate actual waste in kg
    df_processed['rice_waste_kg'] = df_processed['rice_prepared_kg'] * (df_processed['rice_waste_percent'] / 100)
    df_processed['meat_waste_kg'] = df_processed['meat_prepared_kg'] * (df_processed['meat_waste_percent'] / 100)
    df_processed['veg_waste_kg'] = df_processed['veg_prepared_kg'] * (df_processed['veg_waste_percent'] / 100)
    
    # Calculate wasted bowls equivalent
    df_processed['wasted_bowls'] = df_processed[['rice_waste_percent', 'meat_waste_percent', 'veg_waste_percent']].mean(axis=1) * df_processed['bowls_prepared'] / 100
    
    return df_processed

def create_food_waste_pie_chart(selected_data):
    # Calculate consumed amounts
    rice_consumed = selected_data['rice_prepared_kg'] * (1 - selected_data['rice_waste_percent']/100)
    meat_consumed = selected_data['meat_prepared_kg'] * (1 - selected_data['meat_waste_percent']/100)
    veg_consumed = selected_data['veg_prepared_kg'] * (1 - selected_data['veg_waste_percent']/100)
    
    # Get waste amounts
    rice_wasted = selected_data['rice_waste_kg']
    meat_wasted = selected_data['meat_waste_kg']
    veg_wasted = selected_data['veg_waste_kg']
    
    values = [
        rice_consumed, rice_wasted,
        meat_consumed, meat_wasted,
        veg_consumed, veg_wasted
    ]
    
    labels = [
        'Rice (Consumed)', 'Rice (Wasted)',
        'Meat (Consumed)', 'Meat (Wasted)',
        'Vegetables (Consumed)', 'Vegetables (Wasted)'
    ]
    
    colors = [
        COLORS['rice']['dark'], COLORS['rice']['light'],
        COLORS['meat']['dark'], COLORS['meat']['light'],
        COLORS['veg']['dark'], COLORS['veg']['light']
    ]
    
    fig = go.Figure(data=[go.Pie(
        values=values,
        labels=labels,
        hole=0.3,
        marker_colors=colors,
        rotation=14,
        sort=False,
        textinfo='percent',
        textposition='inside'
    )])
    
    fig.update_layout(
        title="Food Distribution (Consumed vs Wasted)",
        annotations=[
            dict(
                text=f"Total: {selected_data['bowl_weight_kg'] * selected_data['bowls_prepared']:.1f}kg",
                x=0.5, y=0.5,
                font_size=20,
                showarrow=False
            )
        ],
        height=600,
        legend=dict(
            orientation="h",
            yanchor="bottom",
            y=-0.2,
            xanchor="center",
            x=0.5
        )
    )
    
    return fig

def create_individual_pie_charts(selected_data):
    figures = {}
    
    food_types = [
        ('Rice', selected_data['rice_prepared_kg'], selected_data['rice_waste_kg'], 
         COLORS['rice']['dark'], COLORS['rice']['light']),
        ('Meat', selected_data['meat_prepared_kg'], selected_data['meat_waste_kg'], 
         COLORS['meat']['dark'], COLORS['meat']['light']),
        ('Vegetables', selected_data['veg_prepared_kg'], selected_data['veg_waste_kg'], 
         COLORS['veg']['dark'], COLORS['veg']['light'])
    ]
    
    for food_type, prepared, wasted, color_prep, color_waste in food_types:
        consumed = prepared - wasted
        
        fig = go.Figure(data=[go.Pie(
            labels=['Consumed', 'Wasted'],
            values=[consumed, wasted],
            hole=.3,
            marker_colors=[color_prep, color_waste],
            title=f"{food_type}<br>({wasted/prepared*100:.1f}% wasted)"
        )])
        
        fig.update_layout(
            showlegend=True,
            height=300,
            margin=dict(t=40, b=0, l=0, r=0)
        )
        
        figures[food_type] = fig
    
    return figures

def main():
    st.set_page_config(page_title="Food Waste Dashboard", layout="wide")
    
    try:
        df = load_data()
    except Exception as e:
        st.error(f"Error loading data: {str(e)}")
        return
    
    st.sidebar.title("Stall Selection")
    stall_ids = df['stall'].unique()
    selected_stall_id = st.sidebar.selectbox(
        "Choose a stall ID",
        stall_ids
    )
    
    df_stall = df[df['stall'] == selected_stall_id]
    
    st.title(f"Food Waste Dashboard - Stall #{selected_stall_id}")
    
    # Add refresh button
    if st.button("Refresh Data"):
        st.cache_data.clear()
        st.rerun()
    
    st.subheader("Daily Waste Breakdown")
    
    view_mode = st.radio("View Mode", ["Percentage", "Kilograms", "Bowls"])
    
    if view_mode == "Percentage":
        fig = px.bar(df_stall, x='date', 
                     y=['rice_waste_percent', 'meat_waste_percent', 'veg_waste_percent'],
                     title="Food Waste by Category (%)",
                     labels={'value': 'Waste (%)', 'variable': 'Category'},
                     barmode='group',
                     color_discrete_map={
                         'rice_waste_percent': COLORS['rice']['light'],
                         'meat_waste_percent': COLORS['meat']['light'],
                         'veg_waste_percent': COLORS['veg']['light']
                     })
    elif view_mode == "Kilograms":
        fig = px.bar(df_stall, x='date', 
                     y=['rice_waste_kg', 'meat_waste_kg', 'veg_waste_kg'],
                     title="Food Waste by Category (kg)",
                     labels={'value': 'Waste (kg)', 'variable': 'Category'},
                     barmode='group',
                     color_discrete_map={
                         'rice_waste_kg': COLORS['rice']['light'],
                         'meat_waste_kg': COLORS['meat']['light'],
                         'veg_waste_kg': COLORS['veg']['light']
                     })
    else:
        fig = px.bar(df_stall, x='date',
                     y=['bowls_prepared', 'wasted_bowls'],
                     title="Bowls Prepared vs Wasted",
                     labels={'value': 'Number of Bowls', 'variable': 'Category'},
                     barmode='group')
    
    st.plotly_chart(fig, use_container_width=True)
    
    st.subheader("Daily Food Waste Analysis")
    selected_date = st.selectbox(
        "Select a day to see detailed breakdown:",
        df_stall['date'].dt.strftime('%Y-%m-%d').unique()
    )
    
    selected_data = df_stall[df_stall['date'].dt.strftime('%Y-%m-%d') == selected_date].iloc[0]
    
    col1, col2 = st.columns(2)
    
    with col1:
        st.markdown(f"""
        **Bowl Information:**
        - Total bowls prepared: {selected_data['bowls_prepared']}
        - Bowl composition:
            - Rice: 50%
            - Meat: 25%
            - Vegetables: 25%
        - Total weight per bowl: {selected_data['bowl_weight_kg']*1000:.0f}g
        """)
    
    with col2:
        st.markdown(f"""
        **Waste Percentages:**
        - Rice: {selected_data['rice_waste_percent']:.1f}%
        - Meat: {selected_data['meat_waste_percent']:.1f}%
        - Vegetables: {selected_data['veg_waste_percent']:.1f}%
        """)
    
    st.subheader("Overall Distribution")
    fig = create_food_waste_pie_chart(selected_data)
    st.plotly_chart(fig, use_container_width=True)
    
    st.subheader("Individual Component Analysis")
    pie_charts = create_individual_pie_charts(selected_data)
    
    cols = st.columns(3)
    for i, (food_type, fig) in enumerate(pie_charts.items()):
        with cols[i]:
            st.plotly_chart(fig, use_container_width=True)
    
    total_prepared = selected_data['bowl_weight_kg'] * selected_data['bowls_prepared']
    total_wasted = (selected_data['rice_waste_kg'] + 
                    selected_data['meat_waste_kg'] + 
                    selected_data['veg_waste_kg'])
    
    st.markdown(f"""
    **Waste Summary for {selected_date}:**
    - Total food prepared: {total_prepared:.2f} kg
    - Total food wasted: {total_wasted:.2f} kg
    - Overall waste percentage: {(total_wasted/total_prepared)*100:.1f}%
    - Cost of wasted food: ${total_wasted * selected_data['cost_per_kg']:.2f}
    """)

if __name__ == "__main__":
    main()